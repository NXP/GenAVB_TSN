#! /bin/sh

PACKAGE=ENDPOINT

. "/etc/genavb/config"

PTP_APP="fgptp"
PTP_APP_BR="fgptp-br"
PTP_CFG_FILE=/etc/genavb/fgptp.cfg
PTP_CFG_FILE_BR=/etc/genavb/fgptp-br.cfg
PIDFILE=/var/run/$PTP_APP.pid
PIDFILE_BR=/var/run/$PTP_APP_BR.pid

if [ -z $AVB_MODE ]; then
	AVB_MODE=0
fi

# default configuration matching most common machines
ITF=eth0
PTP_PRIO=61

if [ "$AVB_MODE" -eq 1 ]; then
	NET_AVB_MODULE=1
else
	NET_AVB_MODULE=0
fi

if [ $PACKAGE = "ENDPOINT" ] || [ $PACKAGE = "HYBRID" ]; then
	SET_AFFINITY=1
else
	SET_AFFINITY=0
fi

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
	elif grep -q 'LS1028A RDB Board' /sys/devices/soc0/machine || grep -q 'LS1028ARDB' /etc/hostname; then
		echo 'LS1028A'
	else
		echo 'SabreAI'
	fi
}

set_machine_variables()
{
# Some platform-dependant variables.
case $1 in
'imx8mpevk')
	if [ "$AVB_MODE" -eq 0 ]; then
		ITF=eth1
	fi
	;;
'LS1028A')
	ITF=eno2
	;;
*)
	;;
esac

}

gptp_ipc_nodes()
{
	major=$(grep ipcdrv /proc/devices | awk '{ print $1 }' -)

	rm -fr /dev/ipc_media_stack_gptp_rx
	rm -fr /dev/ipc_media_stack_gptp_tx
	rm -fr /dev/ipc_gptp_media_stack_rx
	rm -fr /dev/ipc_gptp_media_stack_tx
	rm -fr /dev/ipc_gptp_media_stack_sync_rx
	rm -fr /dev/ipc_gptp_media_stack_sync_tx

	mknod /dev/ipc_media_stack_gptp_rx c "$major" 46
	mknod /dev/ipc_media_stack_gptp_tx c "$major" 47
	mknod /dev/ipc_gptp_media_stack_rx c "$major" 148
	mknod /dev/ipc_gptp_media_stack_tx c "$major" 149
	mknod /dev/ipc_gptp_media_stack_sync_rx c "$major" 150
	mknod /dev/ipc_gptp_media_stack_sync_tx c "$major" 151
}

mac_service_ipc_nodes()
{
	major=$(grep ipcdrv /proc/devices | awk '{ print $1 }' -)

	rm -fr /dev/ipc_media_stack_mac_service_rx
	rm -fr /dev/ipc_media_stack_mac_service_tx
	rm -fr /dev/ipc_mac_service_media_stack_rx
	rm -fr /dev/ipc_mac_service_media_stack_tx
	rm -fr /dev/ipc_mac_service_media_stack_sync_rx
	rm -fr /dev/ipc_mac_service_media_stack_sync_tx

	mknod /dev/ipc_media_stack_mac_service_rx c "$major" 58
	mknod /dev/ipc_media_stack_mac_service_tx c "$major" 59
	mknod /dev/ipc_mac_service_media_stack_rx c "$major" 160
	mknod /dev/ipc_mac_service_media_stack_tx c "$major" 161
	mknod /dev/ipc_mac_service_media_stack_sync_rx c "$major" 162
	mknod /dev/ipc_mac_service_media_stack_sync_tx c "$major" 163
}

bridge_ipc_nodes()
{
	major=$(grep ipcdrv /proc/devices | awk '{ print $1 }' -)

	rm -fr /dev/ipc_media_stack_gptp_bridge_rx
	rm -fr /dev/ipc_media_stack_gptp_bridge_tx
	rm -fr /dev/ipc_gptp_bridge_media_stack_rx
	rm -fr /dev/ipc_gptp_bridge_media_stack_tx
	rm -fr /dev/ipc_gptp_bridge_media_stack_sync_rx
	rm -fr /dev/ipc_gptp_bridge_media_stack_sync_tx

	rm -fr /dev/ipc_media_stack_mac_service_bridge_rx
	rm -fr /dev/ipc_media_stack_mac_service_bridge_tx
	rm -fr /dev/ipc_mac_service_bridge_media_stack_rx
	rm -fr /dev/ipc_mac_service_bridge_media_stack_tx
	rm -fr /dev/ipc_mac_service_bridge_media_stack_sync_rx
	rm -fr /dev/ipc_mac_service_bridge_media_stack_sync_tx

	mknod /dev/ipc_media_stack_gptp_bridge_rx c "$major" 52
	mknod /dev/ipc_media_stack_gptp_bridge_tx c "$major" 53
	mknod /dev/ipc_gptp_bridge_media_stack_rx c "$major" 154
	mknod /dev/ipc_gptp_bridge_media_stack_tx c "$major" 155
	mknod /dev/ipc_gptp_bridge_media_stack_sync_rx c "$major" 156
	mknod /dev/ipc_gptp_bridge_media_stack_sync_tx c "$major" 157

	mknod /dev/ipc_media_stack_mac_service_bridge_rx c "$major" 76
	mknod /dev/ipc_media_stack_mac_service_bridge_tx c "$major" 77
	mknod /dev/ipc_mac_service_bridge_media_stack_rx c "$major" 178
	mknod /dev/ipc_mac_service_bridge_media_stack_tx c "$major" 179
	mknod /dev/ipc_mac_service_bridge_media_stack_sync_rx c "$major" 180
	mknod /dev/ipc_mac_service_bridge_media_stack_sync_tx c "$major" 181
}

load_genavb_module()
{
	insmod /lib/modules/"$(uname -r)"/genavb/avb.ko > /dev/null 2>&1

	if [ $NET_AVB_MODULE -eq 1  ]; then
		major=$(grep avbdrv /proc/devices | awk '{ print $1 }' -)
		rm -fr /dev/avb
		mknod /dev/avb c "$major" 0

		major=$(grep netdrv /proc/devices | awk '{ print $1 }' -)
		rm -fr /dev/net_rx
		rm -fr /dev/net_tx
		mknod /dev/net_rx c "$major" 0
		mknod /dev/net_tx c "$major" 1
	fi
}

start_gptp_stack()
{
	echo "Starting fgptp endpoint stack"

	gptp_ipc_nodes
	mac_service_ipc_nodes

	if [ -f $PIDFILE ]; then
		ps_list=$(${PS})
		ptp_running=$(echo "$ps_list" | grep $PTP_APP)
		if [ -z "${ptp_running}" ]; then
			rm $PIDFILE
		else
			return
		fi
	fi

	$PTP_APP -f "$PTP_CFG_FILE" > /var/log/$PTP_APP 2>&1 &

	ptp_pid=$!

	echo $ptp_pid >$PIDFILE

	if [ $SET_AFFINITY -ne 0 ]; then
		echo "setting PTP cpu affinity for pid $ptp_pid"
		taskset -p "$CPU_MASK" $ptp_pid
		if [ "$?" != 0 ]; then
			echo "!!! WARNING: Setting PTP cpu affinity failed!"
		fi
	fi

	echo "Setting PTP priority for pid $ptp_pid to $PTP_PRIO"
	chrt -pf $PTP_PRIO $ptp_pid
	if [ "$?" != 0 ]; then
		echo "!!! WARNING: Setting PTP priority to $PTP_PRIO failed!"
	fi

	if [ "$CFG_USE_PHC2SYS" -eq 1 ]; then
		ps_list=$(${PS})
		phc_running=$(echo "$ps_list" | grep phc2sys)
		if [ -z "${phc_running}" ]; then
			echo "Starting phc2sys"
			taskset "$CPU_MASK" phc2sys -s ${ITF} -O 0 -S 0.00002 &
		fi
	fi
}

start_gptp_stack_br()
{
	echo "Starting fgptp bridge stack"

	bridge_ipc_nodes

	if [ -f $PIDFILE_BR ]; then
		ps_list=$(${PS})
		ptp_running=$(echo "$ps_list" | grep $PTP_APP_BR)
		if [ -z "${ptp_running}" ]; then
			rm $PIDFILE_BR
		else
			return
		fi
	fi

	$PTP_APP -b -f "$PTP_CFG_FILE_BR" > /var/log/$PTP_APP_BR 2>&1 &

	ptp_br_pid=$!

	echo $ptp_br_pid >$PIDFILE_BR

	if [ $SET_AFFINITY -ne 0 ]; then
		echo "setting PTP bridge cpu affinity for pid $ptp_br_pid"
		taskset -p "$CPU_MASK" $ptp_br_pid
		if [ "$?" != 0 ]; then
			echo "!!! WARNING: Setting PTP bridge cpu affinity failed!"
		fi
	fi

	echo "Setting PTP priority for pid $ptp_br_pid to $PTP_PRIO"
	chrt -pf $PTP_PRIO $ptp_br_pid
	if [ "$?" != 0 ]; then
		echo "!!! WARNING: Setting PTP bridge priority to $PTP_PRIO failed!"
	fi
}

kill_process_pidfile()
{
	if [ -f "${1}" ]; then
		kill "$(cat ${1})" > /dev/null 2>&1
		rm "${1}"
	fi
}

stop_gptp_stack()
{
	echo "Stopping fgptp endpoint stack"

	kill_process_pidfile $PIDFILE
}

stop_gptp_stack_br()
{
	echo "Stopping fgptp bridge stack"

	kill_process_pidfile $PIDFILE_BR
}

start_br()
{
	load_genavb_module

	if [ $PACKAGE = "BRIDGE" ] || [ $PACKAGE = "HYBRID" ]; then
		start_gptp_stack_br
	fi
}

stop_br()
{
	if [ $PACKAGE = "BRIDGE" ] || [ $PACKAGE = "HYBRID" ]; then
		stop_gptp_stack_br
	fi
}

start_ep()
{
	load_genavb_module

	if [ $PACKAGE = "ENDPOINT" ] || [ $PACKAGE = "HYBRID" ] || [ $PACKAGE = "ENDPOINT_GPTP" ]; then
		start_gptp_stack
	fi
}

stop_ep()
{
	if [ $PACKAGE = "ENDPOINT" ] || [ $PACKAGE = "HYBRID" ] || [ $PACKAGE = "ENDPOINT_GPTP" ]; then
		stop_gptp_stack
	fi
}

start()
{
	load_genavb_module

	if [ $PACKAGE = "BRIDGE" ] || [ $PACKAGE = "HYBRID" ]; then
		start_gptp_stack_br
	fi

	if [ $PACKAGE = "ENDPOINT" ] || [ $PACKAGE = "HYBRID" ] || [ $PACKAGE = "ENDPOINT_GPTP" ]; then
		start_gptp_stack
	fi
}

stop()
{
	if [ $PACKAGE = "BRIDGE" ] || [ $PACKAGE = "HYBRID" ]; then
		stop_gptp_stack_br
	fi

	if [ $PACKAGE = "ENDPOINT" ] || [ $PACKAGE = "HYBRID" ] || [ $PACKAGE = "ENDPOINT_GPTP" ]; then
		stop_gptp_stack
	fi
}

############################ MAIN ############################

PATH=/sbin:/usr/sbin:/bin:/usr/bin:.

# Check which ps binary is in use (busybox or procps)
ps -V 2>&1 |grep -q procps && PS='ps ax' || PS='ps'
echo "Using ps = ${PS}"

NB_CPU=$(grep -c processor /proc/cpuinfo)
if  [ "$NB_CPU" -ge 2 ];then
	CPU_MASK=2
else
	CPU_MASK=1
fi

# Detect the platform we are running on and set $MACHINE accordingly, then set variables.
MACHINE=$(detect_machine)

set_machine_variables "$MACHINE"

case "$1" in
start_ep)
	start_ep
	;;
start_br)
	start_br
	;;
start)
	start
	;;
stop_ep)
	stop_ep
	;;
stop_br)
	stop_br
	;;
stop)
	stop
	;;
restart)
	stop_gptp_stack; stop_gptp_stack_br ; sleep 1; start_gptp_stack_br; start_gptp_stack
	;;
*)
	echo "Usage: $0 start|stop|restart|start_ep|start_br|stop_ep|stop_br" >&2
	exit 3
	;;
esac
