#! /bin/sh

PACKAGE=BRIDGE

source "/etc/genavb/config"

if [ $PACKAGE == "ENDPOINT" ] || [ $PACKAGE == "HYBRID" ]; then
	source $APPS_CFG_FILE
fi

# Detect the machine we are running on.
# Possible echoed values are:
# - SabreAI (default)
# - Vybrid
# - SabreSD
detect_machine ()
{
	cat /proc/cpuinfo | grep "Vybrid VF610" > /dev/null
	if [ "$?" == 0 ]; then
		echo 'Vybrid'
	elif grep -q 'sac58r' /proc/cpuinfo; then
		echo 'sac58r'
	elif grep -q 'Freescale i\.MX6 .* SABRE Smart Device Board' /sys/devices/soc0/machine; then
		echo 'SabreSD'
	elif grep -q 'Freescale i\.MX6 SoloX SDB .* Board' /sys/devices/soc0/machine; then
		echo 'SxSabreSD'
	elif grep -q 'NXP i.MX6 SoloX Broadcast Smart Antenna Board' /sys/devices/soc0/machine; then
		echo 'SxSmartAntenna'
	elif grep -q 'Freescale i\.MX6 ULL 14x14 EVK Board' /sys/devices/soc0/machine; then
		echo 'imx6ullEvkBoard'
	elif grep -q 'Freescale i\.MX8MQ EVK' /sys/devices/soc0/machine; then
		echo 'imx8mqevk'
	elif grep -q 'FSL i\.MX8MM EVK board' /sys/devices/soc0/machine; then
		echo 'imx8mmevk'
	elif grep -q 'NXP i\.MX8MPlus EVK board' /sys/devices/soc0/machine; then
		echo 'imx8mpevk'
	elif grep -q 'LS1028A RDB Board' /sys/devices/soc0/machine | grep -q 'LS1028ARDB' /etc/hostname; then
		echo 'LS1028A'
	else
		echo 'SabreAI'
	fi
}

set_machine_variables()
{
# Some platform-dependant variables.
case $1 in
'Vybrid')
	# vf610 case
	IRQ_DMA_NAME="eDMA tx"
	IRQ_TIMER_NAME="ftm"
	IRQ_ETH_NAME="ethernet"
	CFG_EXTERNAL_CONTROLS_APP_OPT="-m DAC1"
	SND_SOC_CARD_NAME="cs42xx8-audio"
	;;
'SabreSD'|'SxSabreSD')
	# Sabre SD
	IRQ_DMA_NAME="sdma"
	IRQ_TIMER_NAME="epit"
	IRQ_ETH_NAME="ethernet"
	CFG_EXTERNAL_CONTROLS_APP_OPT="-m Speaker"
	SND_SOC_CARD_NAME="wm8962-audio"
	;;
'sac58r')
	# sac58r (Rayleigh) cases
	IRQ_DMA_NAME="eDMA tx"
	IRQ_TIMER_NAME="ftm"
	IRQ_ETH_NAME="ethernet"
	CFG_EXTERNAL_CONTROLS_APP_OPT="-m DAC1"
	SND_SOC_CARD_NAME="imx-audiocodec"
	;;
'SxSmartAntenna')
	# NXP i.MX6 SoloX Broadcast Smart Antenna Board
	IRQ_DMA_NAME="sdma"
	IRQ_TIMER_NAME="epit"
	IRQ_ETH_NAME="ethernet"
	CFG_EXTERNAL_CONTROLS_APP_OPT="-m DAC1"
	SND_SOC_CARD_NAME="NONE"
	;;
'imx6ullEvkBoard')
	# iMX.6 UltraLiteLite EVK Board
	IRQ_DMA_NAME="sdma"
	IRQ_TIMER_NAME="epit"
	IRQ_ETH_NAME="ethernet"
	CFG_EXTERNAL_CONTROLS_APP_OPT="-m Headphone"
	SND_SOC_CARD_NAME="wm8960-audio"
	;;
'imx8mqevk')
	# NXP i.MX8MQ EVK Board
	IRQ_DMA_NAME="sdma"
	IRQ_TIMER_NAME="gpt"
	IRQ_ETH_NAME="ethernet"
	CFG_CONTROLLER_APP_MACH_OPT_WL="-e /dev/input/event1"
	SND_SOC_CARD_NAME="wm8524-audio"
	# This board does not have an alsa control.
	# Pass dummy as alsa control to make the controls app open the controlled socket anyway
	CFG_EXTERNAL_CONTROLS_APP_OPT="-m dummy"
	# Set Primary video device
	if [ -z "$CFG_PRIMARY_VIDEO_DEVICE" ] || [ "$CFG_PRIMARY_VIDEO_DEVICE" == "default" ]; then
		CFG_PRIMARY_VIDEO_DEVICE="hdmi"
	fi
	# Set MJPEG minimum latency
	if [ -z "$CFG_TOTAL_MJPEG_LATENCY" ] || [ "$CFG_TOTAL_MJPEG_LATENCY" == "default" ]; then
		CFG_TOTAL_MJPEG_LATENCY="74000000"
	fi
	;;
'imx8mmevk')
	# NXP i.MX8MM EVK Board
	IRQ_DMA_NAME="sdma"
	IRQ_TIMER_NAME="gpt"
	IRQ_ETH_NAME="ethernet"
	CFG_CONTROLLER_APP_MACH_OPT_WL="-e /dev/input/event1"
	SND_SOC_CARD_NAME="wm8524-audio"
	# This board does not have an alsa control.
	# Pass dummy as alsa control to make the controls app open the controlled socket anyway
	CFG_EXTERNAL_CONTROLS_APP_OPT="-m dummy"
	# Set Primary video device
	if [ -z "$CFG_PRIMARY_VIDEO_DEVICE" ] || [ "$CFG_PRIMARY_VIDEO_DEVICE" == "default" ]; then
		CFG_PRIMARY_VIDEO_DEVICE="hdmi"
	fi
	# Set MJPEG minimum latency
	if [ -z "$CFG_TOTAL_MJPEG_LATENCY" ] || [ "$CFG_TOTAL_MJPEG_LATENCY" == "default" ]; then
		CFG_TOTAL_MJPEG_LATENCY="74000000"
	fi
	;;
'imx8mpevk')
	# NXP i.MX8MPlus EVK Board
	ITF=eth1
	IRQ_ETH_NAME="eth1"
	NET_AVB_MODULE=0
	FORCE_100M=0
	;;
'LS1028A')
	ITF=eno2
	SET_AFFINITY=0
	FORCE_100M=0
	NET_AVB_MODULE=0
	RT_THROTTLING=0
	;;
'SabreAI'|*)
	# Sabre AI, imx6q (and others?) cases
	IRQ_DMA_NAME="sdma"
	IRQ_TIMER_NAME="epit"
	IRQ_ETH_NAME="ethernet"
	CFG_EXTERNAL_CONTROLS_APP_OPT="-m DAC1"
	CFG_CONTROLLER_APP_MACH_OPT_WL="-e /dev/input/event0"
	SND_SOC_CARD_NAME="cs42888-audio"
	if [ -z "$CFG_PRIMARY_VIDEO_DEVICE" ] || [ "$CFG_PRIMARY_VIDEO_DEVICE" == "default" ]; then
		CFG_PRIMARY_VIDEO_DEVICE="lvds"
	fi
	# Set MJPEG minimum latency
	if [ -z "$CFG_TOTAL_MJPEG_LATENCY" ] || [ "$CFG_TOTAL_MJPEG_LATENCY" == "default" ]; then
		CFG_TOTAL_MJPEG_LATENCY="41000000"
	fi
	;;
esac

}

print_variables()
{
	echo "GENAVB_CFG_FILE               " "$GENAVB_CFG_FILE"
	echo "APPS_CFG_FILE                 " "$APPS_CFG_FILE"
	echo "CFG_USE_EXTERNAL_CONTROLS     " "$CFG_USE_EXTERNAL_CONTROLS"
	echo "CFG_EXTERNAL_CONTROLS_APP     " "$CFG_EXTERNAL_CONTROLS_APP"
	echo "CFG_EXTERNAL_CONTROLS_APP_OPT " "$CFG_EXTERNAL_CONTROLS_APP_OPT"
	echo "CFG_CONTROLLER_FB             " "$CFG_CONTROLLER_FB"
	echo "CFG_CONTROLLER_FBSET_OPT      " "$CFG_CONTROLLER_FBSET_OPT"
	echo "CFG_CONTROLLER_APP            " "$CFG_CONTROLLER_APP"
	echo "CFG_CONTROLLER_APP_OPT_FB     " "$CFG_CONTROLLER_APP_OPT_FB"
	echo "CFG_CONTROLLER_APP_OPT_WL     " "$CFG_CONTROLLER_APP_OPT_WL"
	echo "CFG_CONTROLLER_APP_MACH_OPT_WL   " "$CFG_CONTROLLER_APP_MACH_OPT_WL"
	echo "CFG_EXTERNAL_MEDIA_APP        " "$CFG_EXTERNAL_MEDIA_APP"
	echo "CFG_EXTERNAL_MEDIA_APP_OPT    " "$CFG_EXTERNAL_MEDIA_APP_OPT"
}

gst_plugin_setup()
{
	echo "Generating gst plugins registry file"

	if [ -f $GST_INSPECT1 ]; then
		$GST_INSPECT1 > /dev/null
	fi

	export GST_REGISTRY_UPDATE="no"
}

# 'Default' alsa mixer setup.
alsa_mixer_setup()
{
	aplay -l | grep $SND_SOC_CARD_NAME > /dev/null;

	if [ $? -ne 0 ]; then
		echo "no known sound card registered"
	else 
		case "$MACHINE" in
		'SabreSD')
			# Playback
			amixer -q sset Speaker 150,150
			amixer -q sset Headphone 70,70
			# TODO: setup capture mixers, too.
			;;
		'SxSabreSD')
			# Playback
			amixer -q sset Speaker 150,150
			amixer -q sset Headphone 100,100
			# Capture
			amixer -q sset Capture 0,0
			;;
		'imx6ullEvkBoard')
			# Playback
			amixer -q sset Speaker 150,150
			amixer -q sset Headphone 100,100
			# Capture: Enable MAIN MIC Channels
			amixer -q cset name='Right Input Mixer Boost Switch' on
			amixer -q cset name='Right Boost Mixer RINPUT1 Switch' on
			amixer -q cset name='Right Boost Mixer RINPUT2 Switch' on
			amixer -q cset name='Right Boost Mixer RINPUT3 Switch' off
			amixer -q cset name='ADC PCM Capture Volume' 220
			# Capture: Using MAIN MIC to support stereo
			amixer -q cset name='ADC Data Output Select' 2
			;;
		'imx8mqevk'|'imx8mmevk')
			#The imx8mqevk and imx8mmevk boards do not have alsa sound controls
			echo "This board: $MACHINE Does not have alsa sound controls for sound card: $SND_SOC_CARD_NAME"
			return
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

		echo Alsa mixer setup done for $SND_SOC_CARD_NAME
	fi
}

stop_gptp_stack()
{
	fgptp.sh stop
}

start_gptp_stack()
{
	fgptp.sh start
}

avdecc_ipc_nodes()
{
	major=`grep ipcdrv /proc/devices | awk '{ print $1 }' -`

	rm -fr /dev/ipc_avdecc_srp_rx
	rm -fr /dev/ipc_avdecc_srp_tx

	rm -fr /dev/ipc_avdecc_media_stack_rx
	rm -fr /dev/ipc_avdecc_media_stack_tx
	rm -fr /dev/ipc_media_stack_avdecc_rx
	rm -fr /dev/ipc_media_stack_avdecc_tx

	rm -fr /dev/ipc_avdecc_maap_rx
	rm -fr /dev/ipc_avdecc_maap_tx

	rm -fr /dev/ipc_avdecc_controlled_rx
	rm -fr /dev/ipc_avdecc_controlled_tx
	rm -fr /dev/ipc_controlled_avdecc_rx
	rm -fr /dev/ipc_controlled_avdecc_tx

	rm -fr /dev/ipc_avdecc_controller_rx
	rm -fr /dev/ipc_avdecc_controller_tx
	rm -fr /dev/ipc_controller_avdecc_rx
	rm -fr /dev/ipc_controller_avdecc_tx
	rm -fr /dev/ipc_avdecc_controller_sync_rx
	rm -fr /dev/ipc_avdecc_controller_sync_tx

	mknod /dev/ipc_avdecc_srp_rx c $major 0
	mknod /dev/ipc_avdecc_srp_tx c $major 1

	mknod /dev/ipc_avdecc_media_stack_rx c $major 2
	mknod /dev/ipc_avdecc_media_stack_tx c $major 3
	mknod /dev/ipc_media_stack_avdecc_rx c $major 4
	mknod /dev/ipc_media_stack_avdecc_tx c $major 5

	mknod /dev/ipc_avdecc_maap_rx c $major 6
	mknod /dev/ipc_avdecc_maap_tx c $major 7

	mknod /dev/ipc_avdecc_controlled_rx c $major 8
	mknod /dev/ipc_avdecc_controlled_tx c $major 9
	mknod /dev/ipc_controlled_avdecc_rx c $major 10
	mknod /dev/ipc_controlled_avdecc_tx c $major 11

	mknod /dev/ipc_avdecc_controller_rx c $major 12
	mknod /dev/ipc_avdecc_controller_tx c $major 13
	mknod /dev/ipc_controller_avdecc_rx c $major 14
	mknod /dev/ipc_controller_avdecc_tx c $major 15
	mknod /dev/ipc_avdecc_controller_sync_rx c $major 16
	mknod /dev/ipc_avdecc_controller_sync_tx c $major 17
}

avtp_ipc_nodes()
{
	major=`grep ipcdrv /proc/devices | awk '{ print $1 }' -`

	rm -fr /dev/ipc_media_stack_avtp_rx
	rm -fr /dev/ipc_media_stack_avtp_tx
	rm -fr /dev/ipc_avtp_media_stack_rx
	rm -fr /dev/ipc_avtp_media_stack_tx

	rm -fr /dev/ipc_avtp_stats_rx
	rm -fr /dev/ipc_avtp_stats_tx

	mknod /dev/ipc_media_stack_avtp_rx c $major 22
	mknod /dev/ipc_media_stack_avtp_tx c $major 23
	mknod /dev/ipc_avtp_media_stack_rx c $major 124
	mknod /dev/ipc_avtp_media_stack_tx c $major 125

	mknod /dev/ipc_avtp_stats_rx c $major 44
	mknod /dev/ipc_avtp_stats_tx c $major 45
}

srp_ipc_nodes()
{
	major=`grep ipcdrv /proc/devices | awk '{ print $1 }' -`

	rm -fr /dev/ipc_media_stack_msrp_rx
	rm -fr /dev/ipc_media_stack_msrp_tx
	rm -fr /dev/ipc_msrp_media_stack_rx
	rm -fr /dev/ipc_msrp_media_stack_tx
	rm -fr /dev/ipc_msrp_media_stack_sync_rx
	rm -fr /dev/ipc_msrp_media_stack_sync_tx

	rm -fr /dev/ipc_media_stack_mvrp_rx
	rm -fr /dev/ipc_media_stack_mvrp_tx
	rm -fr /dev/ipc_mvrp_media_stack_rx
	rm -fr /dev/ipc_mvrp_media_stack_tx
	rm -fr /dev/ipc_mvrp_media_stack_sync_rx
	rm -fr /dev/ipc_mvrp_media_stack_sync_tx

	mknod /dev/ipc_media_stack_msrp_rx c $major 26
	mknod /dev/ipc_media_stack_msrp_tx c $major 27
	mknod /dev/ipc_msrp_media_stack_rx c $major 128
	mknod /dev/ipc_msrp_media_stack_tx c $major 129
	mknod /dev/ipc_msrp_media_stack_sync_rx c $major 130
	mknod /dev/ipc_msrp_media_stack_sync_tx c $major 131

	mknod /dev/ipc_media_stack_mvrp_rx c $major 32
	mknod /dev/ipc_media_stack_mvrp_tx c $major 33
	mknod /dev/ipc_mvrp_media_stack_rx c $major 134
	mknod /dev/ipc_mvrp_media_stack_tx c $major 135
	mknod /dev/ipc_mvrp_media_stack_sync_rx c $major 136
	mknod /dev/ipc_mvrp_media_stack_sync_tx c $major 137
}

clock_domain_ipc_nodes ()
{
	major=`grep ipcdrv /proc/devices | awk '{ print $1 }' -`

	rm -fr /dev/ipc_media_stack_clock_domain_rx
	rm -fr /dev/ipc_media_stack_clock_domain_tx
	rm -fr /dev/ipc_clock_domain_media_stack_rx
	rm -fr /dev/ipc_clock_domain_media_stack_tx
	rm -fr /dev/ipc_clock_domain_media_stack_sync_rx
	rm -fr /dev/ipc_clock_domain_media_stack_sync_tx

	mknod /dev/ipc_media_stack_clock_domain_rx c $major 38
	mknod /dev/ipc_media_stack_clock_domain_tx c $major 39
	mknod /dev/ipc_clock_domain_media_stack_rx c $major 140
	mknod /dev/ipc_clock_domain_media_stack_tx c $major 141
	mknod /dev/ipc_clock_domain_media_stack_sync_rx c $major 142
	mknod /dev/ipc_clock_domain_media_stack_sync_tx c $major 143
}

bridge_ipc_nodes()
{
	major=`grep ipcdrv /proc/devices | awk '{ print $1 }' -`

	rm -fr /dev/ipc_media_stack_msrp_bridge_rx
	rm -fr /dev/ipc_media_stack_msrp_bridge_tx
	rm -fr /dev/ipc_msrp_bridge_media_stack_rx
	rm -fr /dev/ipc_msrp_bridge_media_stack_tx
	rm -fr /dev/ipc_msrp_bridge_media_stack_sync_rx
	rm -fr /dev/ipc_msrp_bridge_media_stack_sync_tx

	rm -fr /dev/ipc_media_stack_mvrp_bridge_rx
	rm -fr /dev/ipc_media_stack_mvrp_bridge_tx
	rm -fr /dev/ipc_mvrp_bridge_media_stack_rx
	rm -fr /dev/ipc_mvrp_bridge_media_stack_tx
	rm -fr /dev/ipc_mvrp_bridge_media_stack_sync_rx
	rm -fr /dev/ipc_mvrp_bridge_media_stack_sync_tx

	mknod /dev/ipc_media_stack_msrp_bridge_rx c $major 64
	mknod /dev/ipc_media_stack_msrp_bridge_tx c $major 65
	mknod /dev/ipc_msrp_bridge_media_stack_rx c $major 166
	mknod /dev/ipc_msrp_bridge_media_stack_tx c $major 167
	mknod /dev/ipc_msrp_bridge_media_stack_sync_rx c $major 168
	mknod /dev/ipc_msrp_bridge_media_stack_sync_tx c $major 169

	mknod /dev/ipc_media_stack_mvrp_bridge_rx c $major 70
	mknod /dev/ipc_media_stack_mvrp_bridge_tx c $major 71
	mknod /dev/ipc_mvrp_bridge_media_stack_rx c $major 172
	mknod /dev/ipc_mvrp_bridge_media_stack_tx c $major 173
	mknod /dev/ipc_mvrp_bridge_media_stack_sync_rx c $major 174
	mknod /dev/ipc_mvrp_bridge_media_stack_sync_tx c $major 175
}

media_device_nodes()
{
	major=`grep media_drv /proc/devices | awk '{ print $1 }' -`

	rm -fr /dev/media_queue_net
	rm -fr /dev/media_queue_api

	mknod /dev/media_queue_net c $major 0
	mknod /dev/media_queue_api c $major 1
}

mclock_nodes()
{
	major=`grep mclock /proc/devices | awk '{ print $1 }' -`

	rm -fr /dev/mclk_rec_0
	rm -fr /dev/mclk_gen_0
	rm -fr /dev/mclk_ptp_0
	rm -fr /dev/mclk_ptp_1

	mknod /dev/mclk_rec_0 c $major 0
	mknod /dev/mclk_gen_0 c $major 8
	mknod /dev/mclk_ptp_0 c $major 16
	mknod /dev/mclk_ptp_1 c $major 17
}

mtimer_nodes()
{
	major=`grep mtimer /proc/devices | awk '{ print $1 }' -`

	rm -fr /dev/mclk_rec_timer_0
	rm -fr /dev/mclk_gen_timer_0
	rm -fr /dev/mclk_ptp_timer_0
	rm -fr /dev/mclk_ptp_timer_1

	mknod /dev/mclk_rec_timer_0 c $major 0
	mknod /dev/mclk_gen_timer_0 c $major 8
	mknod /dev/mclk_ptp_timer_0 c $major 16
	mknod /dev/mclk_ptp_timer_1 c $major 17
}

load_genavb_module()
{
	insmod /lib/modules/`uname -r`/genavb/avb.ko > /dev/null 2>&1

	if [ $NET_AVB_MODULE -eq 1  ]; then
		major=`grep avbdrv /proc/devices | awk '{ print $1 }' -`
		rm -fr /dev/avb
		mknod /dev/avb c $major 0

		major=`grep netdrv /proc/devices | awk '{ print $1 }' -`
		rm -fr /dev/net_rx
		rm -fr /dev/net_tx
		mknod /dev/net_rx c $major 0
		mknod /dev/net_tx c $major 1
	fi

	if [ $PACKAGE == "ENDPOINT" ] || [ $PACKAGE == "HYBRID" ]; then
		srp_ipc_nodes
	fi

	if [ $TSN -eq 0 ]; then
		if [ $PACKAGE == "ENDPOINT" ] || [ $PACKAGE == "HYBRID" ]; then
			avdecc_ipc_nodes
			avtp_ipc_nodes
			clock_domain_ipc_nodes
			media_device_nodes
			mclock_nodes
			mtimer_nodes
		fi
	fi

	if [ $PACKAGE == "BRIDGE" ] || [ $PACKAGE == "HYBRID" ]; then
		bridge_ipc_nodes
	fi
}

set_irq_priority_and_affinity()
{
	if [ $SET_AFFINITY -eq 0 ]; then
		return
	fi

	# DMA IRQ
	if [ -n "$IRQ_DMA_NAME" ]; then
		irq_nb=`echo $(cat /proc/interrupts | grep $IRQ_DMA_NAME | awk -F: '{ print $1 }')`
		if [ -n "$irq_nb" ]; then
			for _irq_nb in $irq_nb
			do
				irq_pid=$(pgrep -f "irq/$_irq_nb-")
				if [ -n "$irq_pid" ]; then
					echo "Setting IRQ/$_irq_nb (DMA) priority to $IRQ_PRIO, with cpu affinity mask $CPU_MASK"
					chrt -pf $IRQ_PRIO $irq_pid
					[ $NB_CPU -gt 1 ] && echo $CPU_MASK > /proc/irq/$_irq_nb/smp_affinity
				else
					echo "!!! WARNING: Setting IRQ/$_irq_nb (DMA) priority to $IRQ_PRIO failed!"
				fi
			done
		else
			echo "!!! WARNING: There is no matching sdma interrupt!"
		fi
	fi

	# HW TIMER IRQ
	if [ -n "$IRQ_TIMER_NAME" ]; then
		irq_nb=`echo $(cat /proc/interrupts | grep $IRQ_TIMER_NAME | awk -F: '{ print $1 }')`
		if [ -n "$irq_nb" ]; then
			echo "Setting IRQ/$irq_nb (TIMER) cpu affinity mask to $CPU_MASK"
			[ $NB_CPU -gt 1 ] && echo $CPU_MASK > /proc/irq/$irq_nb/smp_affinity
		else
			echo "!!! WARNING: Setting IRQ/$irq_nb (TIMER) cpu affinity mask failed!"
		fi
	fi

	# Genavb timer kernel thread
	kthread_pid=$(pgrep -f 'avb timer')
	if [ -n "$kthread_pid" ]; then
		echo "Setting AVB kthread priority to $KTHREAD_PRIO, with cpu affinity mask $CPU_MASK"
		chrt -pf $KTHREAD_PRIO $kthread_pid
		[ $NB_CPU -gt 1 ] && taskset -p $CPU_MASK $kthread_pid
	else
		echo "!!! WARNING: Setting AVB kthread priority to $KTHREAD_PRIO failed!"
	fi

	# Genavb SJA1105 interface kernel thread
	kthread_pid=$(pgrep -f 'sja1105 tick')
	if [ -n "$kthread_pid" ]; then
		echo "Setting SJA1105 Tick kthread priority to $KTHREAD_PRIO, with cpu affinity mask $CPU_MASK"
		chrt -pf $KTHREAD_PRIO $kthread_pid
		taskset -p $CPU_MASK $kthread_pid
		if [ $? != 0 ]; then
			echo "!!! WARNING: Setting SJA1105 Tick kthread priority to $KTHREAD_PRIO failed!"
		fi
	fi

	# ETHERNET IRQ(S)
	if [ -n "$IRQ_ETH_NAME" ]; then
		irq_nb=$(cat /proc/interrupts | grep $IRQ_ETH_NAME | awk -F: '{ print $1 }')
		if [ -n "$irq_nb" ]; then
			for _irq_nb in $irq_nb
			do
				_irq_nb=`echo $_irq_nb`;
				if [ -n "$_irq_nb" ]; then
					echo "Setting IRQ/$_irq_nb (Ethernet) cpu affinity mask to $CPU_MASK"
					[ $NB_CPU -gt 1 ] && echo $CPU_MASK > /proc/irq/$_irq_nb/smp_affinity
				else
					echo "!!! WARNING: Setting IRQ/$_irq_nb (Ethernet) cpu affinity mask failed!"
				fi
			done
		else
			echo "!!! WARNING: There is no matching ethernet interrupt!"
		fi
	fi
}

# Disable/Enable the cpuidle state, for older kernels
# that do not support Sysfs PM QoS
# $1 : cpu core number
# $2 : disable state entry (0 or 1)
set_cpu_idle_state_disable()
{
	if [ ! -d /sys/devices/system/cpu/cpu$1/cpuidle ]; then
		echo "CPU#$1 idle entry is not present is sysfs"
		return;
	fi

	# Check how many cpuidle states there are
	NB_STATES=`ls -1qd /sys/devices/system/cpu/cpu$1/cpuidle/state* | wc -l`
	for i in $(seq 0 $((NB_STATES-1)))
	do
		_latency=`cat /sys/devices/system/cpu/cpu$1/cpuidle/state$i/latency`
		# Disable/Enable cpu idle states with latency greater thatn 10us
		if [ $_latency -gt 10 ]; then
			echo "Set CPU#$1 idle state$i disable to ($2)"
			echo $2 > /sys/devices/system/cpu/cpu$1/cpuidle/state$i/disable
		fi
	done
}

# Set the CPU power management states
# Enable or Disable cpu idle states for CPU cores
# Running AVB Related tasks
# $1 : 1 (enable) or 0 (disable)
set_cpu_power_management()
{
	if [ "$1" -eq 0 ]; then
		# Disable CPU idle deep state transitions exceeding 10us
		qos_resume_latency=10
		disable=1
	else
		qos_resume_latency=0
		disable=0
	fi

	if [ -f "/sys/devices/system/cpu/cpu$CPU_CORE/power/pm_qos_resume_latency_us" ]; then
		echo "$qos_resume_latency" > /sys/devices/system/cpu/cpu$CPU_CORE/power/pm_qos_resume_latency_us
	else
		set_cpu_idle_state_disable $CPU_CORE $disable
	fi

	if [ $PACKAGE == "ENDPOINT" ] || [ $PACKAGE == "HYBRID" ]; then
		if [ -f "/sys/devices/system/cpu/cpu$MEDIA_APP_CPU_CORE/power/pm_qos_resume_latency_us" ]; then
			echo "$qos_resume_latency" > /sys/devices/system/cpu/cpu$MEDIA_APP_CPU_CORE/power/pm_qos_resume_latency_us
		else
			set_cpu_idle_state_disable $MEDIA_APP_CPU_CORE $disable
		fi
	fi

}

setup()
{
	# Set CPU frequency @528Mhz for imx6ULL
	if [ $MACHINE == "imx6ullEvkBoard" ]; then
		echo userspace > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
		echo 528000 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed
	fi

	if [ $RT_THROTTLING -eq 1 ]; then
		# Disable real-time throttling
		echo -1 > /proc/sys/kernel/sched_rt_runtime_us
	fi

	# Disable CPU idle deep state transitions exceeding 10us
	set_cpu_power_management 0

	if [ $FORCE_100M -eq 1 ]; then
		# Advertise 100 Mbps full-duplex
		ethtool -s ${ITF} advertise 0x8
	fi

	load_genavb_module

	set_irq_priority_and_affinity
}

stop_genavb()
{
	killall -q avb
}

stop_genavb_br()
{
	killall -q avb-br
}

stop_avb_apps()
{
	killall -q `basename $CFG_EXTERNAL_MEDIA_APP`

	if [ "$CFG_USE_EXTERNAL_CONTROLS" -eq 1 ]; then
		killall -q `basename $CFG_EXTERNAL_CONTROLS_APP`
	fi

	if [ -x "`which "$CFG_CONTROLLER_APP" 2> /dev/null`" ]; then
		killall -q `basename $CFG_CONTROLLER_APP`
	fi
}

stop_tsn_apps()
{
	killall -q `basename $CFG_EXTERNAL_MEDIA_APP`
}

stop()
{
	if [ $PACKAGE == "ENDPOINT" ] || [ $PACKAGE == "HYBRID" ]; then
		if [ $TSN -eq 1 ]; then
			stop_tsn_apps
		else
			stop_avb_apps
		fi

		stop_genavb
	fi

	if [ $PACKAGE == "BRIDGE" ] || [ $PACKAGE == "HYBRID" ]; then
		stop_genavb_br
	fi

	# Restore CPU idle deep state transitions latency
	set_cpu_power_management 1
}

start_genavb()
{
	taskset $CPU_MASK avb -f "$GENAVB_CFG_FILE" &> /var/log/avb &
}

start_genavb_br()
{
	taskset $CPU_MASK avb-br -f "$GENAVB_CFG_FILE" &> /var/log/avb-br &
}

start_avb_apps()
{
	# Check if we are under wayland environment or not
	if [ -z "${XDG_RUNTIME_DIR}" ]; then
		echo "No XDG_RUNTIME_DIR set ... No wayland backend"
		WL_BACKEND=0
	else
		echo "XDG_RUNTIME_DIR is set (${XDG_RUNTIME_DIR}) ...  Use wayland backend"
		WL_BACKEND=1
	fi

	alsa_mixer_setup

	gst_plugin_setup

	if [ "$WL_BACKEND" -ne 1 ]; then

		# Disable console blanking, so the display doesn't turn off in the middle of long videos
		echo -e '\033[9;0]\033[14;0]' > /dev/tty0
		# Disable cursor
		echo -e '\033[?25l' > /dev/tty0

		# Detach framebuffer from the console, to stop blinking green top and bottom bars on non-HD screens
		echo 0 >  /sys/class/vtconsole/vtcon1/bind
	fi

	if [ "$CFG_USE_EXTERNAL_CONTROLS" -eq 1 ]; then
		taskset $CPU_MASK $CFG_EXTERNAL_CONTROLS_APP $CFG_EXTERNAL_CONTROLS_APP_OPT &> /var/log/avb_controls_app &
	fi

	# if controller app is required, test if app is executable
	if [ -x "`which "$CFG_CONTROLLER_APP" 2> /dev/null`" ]; then
		if [ "$WL_BACKEND" -ne 1 ]; then
			# blank screen
			cat /dev/zero > /dev/fb0
			# enable fb1
			echo 0 > /sys/class/graphics/$CFG_CONTROLLER_FB/blank
			# Set the screen geometry
			fbset $CFG_CONTROLLER_FBSET_OPT

			taskset $CPU_MASK $CFG_CONTROLLER_APP $CFG_CONTROLLER_APP_OPT_FB &> /var/log/avb_avdecc_controller &
		else
			# Force the legacy wl-shell instead of xdg-shell to avoid touchscreen/surfaces wrong event handling on iMX6
			export QT_WAYLAND_SHELL_INTEGRATION=wl-shell
			taskset $CPU_MASK $CFG_CONTROLLER_APP $CFG_CONTROLLER_APP_OPT_WL $CFG_CONTROLLER_APP_MACH_OPT_WL &> /var/log/avb_avdecc_controller &
		fi
	fi

	# Expand the video device variable
	eval "CFG_EXTERNAL_MEDIA_APP_OPT=\"${CFG_EXTERNAL_MEDIA_APP_OPT}\""

	taskset $MEDIA_APP_CPU_MASK $CFG_EXTERNAL_MEDIA_APP $CFG_EXTERNAL_MEDIA_APP_OPT &> /var/log/avb_media_app &
}

start_tsn_apps()
{
	taskset $MEDIA_APP_CPU_MASK $CFG_EXTERNAL_MEDIA_APP $CFG_EXTERNAL_MEDIA_APP_OPT &
}

start()
{
	echo "Starting GenAVB/TSN $PACKAGE stack"

	stop

	setup

	start_gptp_stack

	if [ $PACKAGE == "BRIDGE" ] || [ $PACKAGE == "HYBRID" ]; then
		start_genavb_br
	fi

	if [ $PACKAGE == "ENDPOINT" ] || [ $PACKAGE == "HYBRID" ]; then
		echo "Starting AVB demo with profile " "$APPS_CFG_FILE"
		echo "Starting GenAVB/TSN stack with configuration file: " "$GENAVB_CFG_FILE"

		start_genavb

		sleep 1 # Work-around cases where the AVB stack isn't ready when the controls app (or media app) starts

		if [ $TSN -eq 1 ]; then
			start_tsn_apps
		else
			start_avb_apps
		fi
	fi
}

############################ MAIN ############################

PATH=/sbin:/usr/sbin:/bin:/usr/bin:.

GST_INSPECT1="/usr/bin/gst-inspect-1.0"

NB_CPU=`cat /proc/cpuinfo | grep processor | wc -l`
if  [ $NB_CPU -ge 2 ];then
	CPU_MASK=2
	CPU_CORE=1
else
	CPU_MASK=1
	CPU_CORE=0
fi

if  [ $NB_CPU -gt 2 ];then
	MEDIA_APP_CPU_MASK=4
	MEDIA_APP_CPU_CORE=2
else
	MEDIA_APP_CPU_MASK=1
	MEDIA_APP_CPU_CORE=0
fi

# Detect the platform we are running on and set $MACHINE accordingly, then set variables.
MACHINE=$(detect_machine)

# default configuration matching most common machines
ITF=eth0
IRQ_PRIO=60
KTHREAD_PRIO=60

SET_AFFINITY=1
FORCE_100M=1
NET_AVB_MODULE=1
RT_THROTTLING=1

set_machine_variables "$MACHINE"

case "$1" in
start)
	#print_variables
	start
	;;
stop)
	stop
	;;
restart)
	stop ; sleep 1; start
	;;
*)
	echo "Usage: $0 start|stop|restart" >&2
	exit 3
	;;
esac
