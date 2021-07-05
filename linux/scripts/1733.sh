#! /bin/sh

PATH=/sbin:/usr/sbin:/bin:/usr/bin:.

ITF=eth0

IRQ_PRIO=60
KTHREAD_PRIO=60

PTP_APP="fgptp"
CFG_USE_PHC2SYS=1

# Detect the machine we are running on.
# Possible echoed values are:
# - SabreAI (default)
# - Vybrid
# - SabreSD
detect_machine ()
{
	if grep -q 'Vybrid VF610' /proc/cpuinfo; then
		echo 'Vybrid'
	elif grep -q 'sac58r' /proc/cpuinfo; then
		echo 'sac58r'
	elif grep -q 'Freescale i\.MX6 .* SABRE Smart Device Board' /sys/devices/soc0/machine; then
		echo 'SabreSD'
	else
		echo 'SabreAI'
	fi
}

# Detect the platform we are running on and set $MACHINE accordingly.
MACHINE=$(detect_machine)

# Some platform-dependant variables.
case "$MACHINE" in
'Vybrid')
	# vf610 case
	IRQ_DMA='40'
	IRQ_TIMER='74'
	IRQ_ETH='110'
	# Alsa volume control
	CFG_EXTERNAL_CONTROLS_APP_OPT="-m DAC1"
	;;
'SabreSD')
	# Sabre SD
	IRQ_DMA='34'
	IRQ_TIMER='88'
	IRQ_ETH='150'
	# Alsa volume control
	CFG_EXTERNAL_CONTROLS_APP_OPT="-m Speaker"
	;;
'sac58r')
	# sac58r (Rayleigh) cases
	IRQ_DMA='40'
	IRQ_TIMER='74'
	IRQ_ETH='75'
	# Alsa volume control
	CFG_EXTERNAL_CONTROLS_APP_OPT="-m DAC1"
	;;
'SabreAI'|*)
	# Sabre AI, imx6q (and others?) cases
	IRQ_DMA='34'
	IRQ_TIMER='88'
	IRQ_ETH='150'
	# Alsa volume control
	CFG_EXTERNAL_CONTROLS_APP_OPT="-m DAC1"
	;;
esac

NB_CPU=$(grep -c processor /proc/cpuinfo)
if  [ "$NB_CPU" -ge 2 ];then
	CPU_MASK=2
else
	CPU_MASK=1
fi


# 'Default' alsa mixer setup.
alsa_mixer_setup()
{
	case "$MACHINE" in
	'SabreSD')
		# Playback
		amixer -q sset Speaker 150,150
		amixer -q sset Headphone 70,70
		# TODO: setup capture mixers, too.
		;;
	'SabreAI'|'Vybrid'|*)
		# Capture
		# 6.0dB Gain
		amixer -q sset ADC1 140,140

		# Muted
		amixer -q sset ADC2 0,0

		# Playback
		# 0.0dB Gain
		amixer -q sset DAC1 255,255

		# Muted
		amixer -q sset DAC2 0,0
		amixer -q sset DAC3 0,0
		amixer -q sset DAC4 0,0
		;;
	esac
}

setup()
{
	ifconfig ${ITF} allmulti

	insmod /lib/modules/"$(uname -r)"/genavb/avb.ko > /dev/null 2>&1

	major=$(grep avbdrv /proc/devices | awk '{ print $1 }' -)
	rm -fr /dev/avb
	mknod /dev/avb c "$major" 0

	major=$(grep netdrv /proc/devices | awk '{ print $1 }' -)
	rm -fr /dev/net_rx
	rm -fr /dev/net_tx
	mknod /dev/net_rx c "$major" 0
	mknod /dev/net_tx c "$major" 1

	if [ "$MACHINE" = "sac58r" ]; then
		major=$(grep mclock /proc/devices | awk '{ print $1 }' -)
		rm -fr /dev/mclk_rec_0
		mknod /dev/mclk_rec_0 c "$major" 0
	fi

	ps_list=$(ps)
	ptp_running=$(echo "$ps_list" | grep $PTP_APP)
	if [ -z "${ptp_running}" ]; then
		echo "Starting $PTP_APP"
		# Use genavb debug level for fgptp
		if [ "$CFG_DEBUG" -eq 1 ]; then
			PTP_OPT="$PTP_OPT $OPT_DEBUG"
		fi

		taskset "$CPU_MASK" $PTP_APP $PTP_OPT -i ${ITF} > /var/log/fgptp 2>&1 &
	fi

	if [ "$CFG_USE_PHC2SYS" -eq 1 ]; then
		phc_running=$(echo "$ps_list" |grep phc2sys)
		if [ -z "${phc_running}" ]; then
			echo "Starting phc2sys"
			taskset "$CPU_MASK" phc2sys -s ${ITF} -O 0 -S 0.00002 &
		fi
	fi

	ptp_pid=$(ps | grep -m1 $PTP_APP | grep -v 'grep' | awk '{ print $1 }' -)
	if [ -n "$ptp_pid" ]; then
		echo "Setting PTP priority to 61"
		chrt -pf 61 "$ptp_pid"
	else
		echo "!!! WARNING: Setting PTP priority to 61 failed!"
	fi

	irq_pid=$(ps |grep -m1 '\[irq/'$IRQ_DMA | grep -v 'grep' | awk '{ print $1 }' -)
	if [ -n "$irq_pid" ]; then
		echo "Setting IRQ/$IRQ_DMA (DMA) priority to $IRQ_PRIO, with cpu affinity mask $CPU_MASK"
		chrt -pf $IRQ_PRIO "$irq_pid"
		echo "$CPU_MASK" > /proc/irq/$IRQ_DMA/smp_affinity
	else
		echo "!!! WARNING: Setting IRQ/40 (DMA) priority to $IRQ_PRIO failed!"
	fi

	irq_pid=$(ps |grep -m1 '\[irq/'$IRQ_TIMER | grep -v 'grep' | awk '{ print $1 }' -)
	if [ -n "$irq_pid" ]; then
		echo "Setting IRQ/$IRQ_TIMER (TIMER) cpu affinity mask to $CPU_MASK"
		echo "$CPU_MASK" > /proc/irq/$IRQ_TIMER/smp_affinity
	else
		echo "!!! WARNING: Setting IRQ/$IRQ_TIMER (TIMER) cpu affinity mask failed!"
	fi

	kthread_pid=$(ps |grep -m1 '\[avb timer\]' | grep -v 'grep' | awk '{ print $1 }' -)
	if [ -n "$kthread_pid" ]; then
		echo "Setting AVB kthread priority to $KTHREAD_PRIO, with cpu affinity mask $CPU_MASK"
		chrt -pf $KTHREAD_PRIO "$kthread_pid"
		taskset -p "$CPU_MASK" "$kthread_pid"
	else
		echo "!!! WARNING: Setting AVB kthread priority to $KTHREAD_PRIO failed!"
	fi

	irq_pid=$(ps |grep -m1 '\[irq/'$IRQ_ETH | grep -v 'grep' | awk '{ print $1 }' -)
	if [ -n "$irq_pid" ]; then
		echo "Setting IRQ/$IRQ_ETH (Ethernet) cpu affinity mask to $CPU_MASK"
		echo "$CPU_MASK" > /proc/irq/$IRQ_ETH/smp_affinity
	else
		echo "!!! WARNING: Setting IRQ/$IRQ_ETH (Ethernet) cpu affinity mask failed!"
	fi

	# Disable console blanking, so the display doesn't turn off in the middle of long videos
	echo -e '\033[9;0]\033[14;0]' > /dev/tty0
	# Disable cursor
	echo -e '\033[?25l' > /dev/tty0
}

alsa_mixer_setup

setup

