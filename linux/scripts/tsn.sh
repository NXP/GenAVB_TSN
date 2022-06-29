#! /bin/sh

PACKAGE=ENDPOINT

. "/etc/genavb/config"

TSN_APP="tsn"
TSN_APP_BR="tsn-br"
PTP_CFG_FILE_0=/etc/genavb/fgptp.cfg
PTP_CFG_FILE_1=/etc/genavb/fgptp.cfg
PTP_CFG_FILE_BR=/etc/genavb/fgptp-br.cfg
PIDFILE_0=/var/run/"$TSN_APP"_0.pid
PIDFILE_1=/var/run/"$TSN_APP"_1.pid
PIDFILE_BR=/var/run/$TSN_APP_BR.pid
# Backward compatibility
FGPTP_APP="fgptp"
FGPTP_APP_BR="fgptp-br"

if [ -z "$AVB_MODE" ]; then
	AVB_MODE=0
fi

# default configuration matching most common machines
ITF=eth0

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

	rm -fr /dev/ipc_media_stack_gptp_1_rx
	rm -fr /dev/ipc_media_stack_gptp_1_tx
	rm -fr /dev/ipc_gptp_1_media_stack_rx
	rm -fr /dev/ipc_gptp_1_media_stack_tx
	rm -fr /dev/ipc_gptp_1_media_stack_sync_rx
	rm -fr /dev/ipc_gptp_1_media_stack_sync_tx

	mknod /dev/ipc_media_stack_gptp_rx c "$major" 46
	mknod /dev/ipc_media_stack_gptp_tx c "$major" 47
	mknod /dev/ipc_gptp_media_stack_rx c "$major" 148
	mknod /dev/ipc_gptp_media_stack_tx c "$major" 149
	mknod /dev/ipc_gptp_media_stack_sync_rx c "$major" 150
	mknod /dev/ipc_gptp_media_stack_sync_tx c "$major" 151

	mknod /dev/ipc_media_stack_gptp_1_rx c "$major" 48
	mknod /dev/ipc_media_stack_gptp_1_tx c "$major" 49
	mknod /dev/ipc_gptp_1_media_stack_rx c "$major" 152
	mknod /dev/ipc_gptp_1_media_stack_tx c "$major" 153
	mknod /dev/ipc_gptp_1_media_stack_sync_rx c "$major" 154
	mknod /dev/ipc_gptp_1_media_stack_sync_tx c "$major" 155
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

	rm -fr /dev/ipc_media_stack_mac_service_1_rx
	rm -fr /dev/ipc_media_stack_mac_service_1_tx
	rm -fr /dev/ipc_mac_service_1_media_stack_rx
	rm -fr /dev/ipc_mac_service_1_media_stack_tx
	rm -fr /dev/ipc_mac_service_1_media_stack_sync_rx
	rm -fr /dev/ipc_mac_service_1_media_stack_sync_tx

	mknod /dev/ipc_media_stack_mac_service_rx c "$major" 58
	mknod /dev/ipc_media_stack_mac_service_tx c "$major" 59
	mknod /dev/ipc_mac_service_media_stack_rx c "$major" 160
	mknod /dev/ipc_mac_service_media_stack_tx c "$major" 161
	mknod /dev/ipc_mac_service_media_stack_sync_rx c "$major" 162
	mknod /dev/ipc_mac_service_media_stack_sync_tx c "$major" 163

	mknod /dev/ipc_media_stack_mac_service_1_rx c "$major" 60
	mknod /dev/ipc_media_stack_mac_service_1_tx c "$major" 61
	mknod /dev/ipc_mac_service_1_media_stack_rx c "$major" 164
	mknod /dev/ipc_mac_service_1_media_stack_tx c "$major" 165
	mknod /dev/ipc_mac_service_1_media_stack_sync_rx c "$major" 166
	mknod /dev/ipc_mac_service_1_media_stack_sync_tx c "$major" 167
}

srp_ipc_nodes()
{
	major=$(grep ipcdrv /proc/devices | awk '{ print $1 }' -)

	rm -fr /dev/ipc_media_stack_msrp_rx
	rm -fr /dev/ipc_media_stack_msrp_tx
	rm -fr /dev/ipc_msrp_media_stack_rx
	rm -fr /dev/ipc_msrp_media_stack_tx
	rm -fr /dev/ipc_msrp_media_stack_sync_rx
	rm -fr /dev/ipc_msrp_media_stack_sync_tx

	rm -fr /dev/ipc_media_stack_msrp_1_rx
	rm -fr /dev/ipc_media_stack_msrp_1_tx
	rm -fr /dev/ipc_msrp_1_media_stack_rx
	rm -fr /dev/ipc_msrp_1_media_stack_tx
	rm -fr /dev/ipc_msrp_1_media_stack_sync_rx
	rm -fr /dev/ipc_msrp_1_media_stack_sync_tx

	rm -fr /dev/ipc_media_stack_mvrp_rx
	rm -fr /dev/ipc_media_stack_mvrp_tx
	rm -fr /dev/ipc_mvrp_media_stack_rx
	rm -fr /dev/ipc_mvrp_media_stack_tx
	rm -fr /dev/ipc_mvrp_media_stack_sync_rx
	rm -fr /dev/ipc_mvrp_media_stack_sync_tx

	rm -fr /dev/ipc_media_stack_mvrp_1_rx
	rm -fr /dev/ipc_media_stack_mvrp_1_tx
	rm -fr /dev/ipc_mvrp_1_media_stack_rx
	rm -fr /dev/ipc_mvrp_1_media_stack_tx
	rm -fr /dev/ipc_mvrp_1_media_stack_sync_rx
	rm -fr /dev/ipc_mvrp_1_media_stack_sync_tx

	mknod /dev/ipc_media_stack_msrp_rx c "$major" 26
	mknod /dev/ipc_media_stack_msrp_tx c "$major" 27
	mknod /dev/ipc_msrp_media_stack_rx c "$major" 128
	mknod /dev/ipc_msrp_media_stack_tx c "$major" 129
	mknod /dev/ipc_msrp_media_stack_sync_rx c "$major" 130
	mknod /dev/ipc_msrp_media_stack_sync_tx c "$major" 131

	mknod /dev/ipc_media_stack_mvrp_rx c "$major" 32
	mknod /dev/ipc_media_stack_mvrp_tx c "$major" 33
	mknod /dev/ipc_mvrp_media_stack_rx c "$major" 134
	mknod /dev/ipc_mvrp_media_stack_tx c "$major" 135
	mknod /dev/ipc_mvrp_media_stack_sync_rx c "$major" 136
	mknod /dev/ipc_mvrp_media_stack_sync_tx c "$major" 137

	mknod /dev/ipc_media_stack_msrp_1_rx c "$major" 28
	mknod /dev/ipc_media_stack_msrp_1_tx c "$major" 29
	mknod /dev/ipc_msrp_1_media_stack_rx c "$major" 126
	mknod /dev/ipc_msrp_1_media_stack_tx c "$major" 127
	mknod /dev/ipc_msrp_1_media_stack_sync_rx c "$major" 132
	mknod /dev/ipc_msrp_1_media_stack_sync_tx c "$major" 133

	mknod /dev/ipc_media_stack_mvrp_1_rx c "$major" 34
	mknod /dev/ipc_media_stack_mvrp_1_tx c "$major" 35
	mknod /dev/ipc_mvrp_1_media_stack_rx c "$major" 138
	mknod /dev/ipc_mvrp_1_media_stack_tx c "$major" 139
	mknod /dev/ipc_mvrp_1_media_stack_sync_rx c "$major" 146
	mknod /dev/ipc_mvrp_1_media_stack_sync_tx c "$major" 147
}

gptp_bridge_ipc_nodes()
{
	major=$(grep ipcdrv /proc/devices | awk '{ print $1 }' -)

	rm -fr /dev/ipc_media_stack_gptp_bridge_rx
	rm -fr /dev/ipc_media_stack_gptp_bridge_tx
	rm -fr /dev/ipc_gptp_bridge_media_stack_rx
	rm -fr /dev/ipc_gptp_bridge_media_stack_tx
	rm -fr /dev/ipc_gptp_bridge_media_stack_sync_rx
	rm -fr /dev/ipc_gptp_bridge_media_stack_sync_tx

	mknod /dev/ipc_media_stack_gptp_bridge_rx c "$major" 52
	mknod /dev/ipc_media_stack_gptp_bridge_tx c "$major" 53
	mknod /dev/ipc_gptp_bridge_media_stack_rx c "$major" 154
	mknod /dev/ipc_gptp_bridge_media_stack_tx c "$major" 155
	mknod /dev/ipc_gptp_bridge_media_stack_sync_rx c "$major" 156
	mknod /dev/ipc_gptp_bridge_media_stack_sync_tx c "$major" 157
}

mac_service_bridge_ipc_nodes()
{
	major=$(grep ipcdrv /proc/devices | awk '{ print $1 }' -)

	rm -fr /dev/ipc_media_stack_mac_service_bridge_rx
	rm -fr /dev/ipc_media_stack_mac_service_bridge_tx
	rm -fr /dev/ipc_mac_service_bridge_media_stack_rx
	rm -fr /dev/ipc_mac_service_bridge_media_stack_tx
	rm -fr /dev/ipc_mac_service_bridge_media_stack_sync_rx
	rm -fr /dev/ipc_mac_service_bridge_media_stack_sync_tx

	mknod /dev/ipc_media_stack_mac_service_bridge_rx c "$major" 76
	mknod /dev/ipc_media_stack_mac_service_bridge_tx c "$major" 77
	mknod /dev/ipc_mac_service_bridge_media_stack_rx c "$major" 178
	mknod /dev/ipc_mac_service_bridge_media_stack_tx c "$major" 179
	mknod /dev/ipc_mac_service_bridge_media_stack_sync_rx c "$major" 180
	mknod /dev/ipc_mac_service_bridge_media_stack_sync_tx c "$major" 181
}

srp_bridge_ipc_nodes()
{
	major=$(grep ipcdrv /proc/devices | awk '{ print $1 }' -)

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

	mknod /dev/ipc_media_stack_msrp_bridge_rx c "$major" 64
	mknod /dev/ipc_media_stack_msrp_bridge_tx c "$major" 65
	mknod /dev/ipc_msrp_bridge_media_stack_rx c "$major" 166
	mknod /dev/ipc_msrp_bridge_media_stack_tx c "$major" 167
	mknod /dev/ipc_msrp_bridge_media_stack_sync_rx c "$major" 168
	mknod /dev/ipc_msrp_bridge_media_stack_sync_tx c "$major" 169

	mknod /dev/ipc_media_stack_mvrp_bridge_rx c "$major" 70
	mknod /dev/ipc_media_stack_mvrp_bridge_tx c "$major" 71
	mknod /dev/ipc_mvrp_bridge_media_stack_rx c "$major" 172
	mknod /dev/ipc_mvrp_bridge_media_stack_tx c "$major" 173
	mknod /dev/ipc_mvrp_bridge_media_stack_sync_rx c "$major" 174
	mknod /dev/ipc_mvrp_bridge_media_stack_sync_tx c "$major" 175
}

load_genavb_module()
{
	avb_loaded=$(lsmod |grep avb)
	if [ -z "${avb_loaded}" ]; then
		insmod /lib/modules/"$(uname -r)"/genavb/avb.ko > /dev/null 2>&1
		if [ $? -ne 0 ]; then
			echo "Failed to load avb kernel module"
			exit 1
		fi
	fi

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

# $1: endpoint number
start_tsn_endpoint_process()
{

	if [ "$1" -eq 1 ]; then
		pid_file=$PIDFILE_1
		endpoint_number=1
		log_file="$TSN_APP-endpoint1"
		gptp_conf_file="$PTP_CFG_FILE_1"
	else
		pid_file=$PIDFILE_0
		endpoint_number=0
		log_file="$TSN_APP-endpoint0"
		gptp_conf_file="$PTP_CFG_FILE_0"
	fi

	if [ -f $pid_file ]; then
		ps_list=$(${PS})
		tsn_running=$(echo "$ps_list" | grep "$TSN_APP.*-e \+$endpoint_number")
		if [ -z "${tsn_running}" ]; then
			rm $pid_file
		else
			return
		fi
	fi

	if [ "$endpoint_number" -eq 1 ]; then
		$TSN_APP -f "$gptp_conf_file" -e $endpoint_number > /var/log/$log_file 2>&1 &

		echo $! > $pid_file
	else
		# Truncate log files to 0 if they exist, otherwise create them.
		# And use append mode with tee and redirection below to assure working log rotation.
		: > /var/log/$TSN_APP
		: > /var/log/$FGPTP_APP

		# Backward compatibility: For endpoint 0, redirect the srp-filtered log to /var/log/fgptp
		($TSN_APP -f "$gptp_conf_file" -e $endpoint_number 2>&1 & echo $! > $pid_file) | tee -a /var/log/$TSN_APP | stdbuf -oL grep -v srp >> /var/log/$FGPTP_APP &

		if [ "$CFG_TSN_MULTI_ENDPOINTS" -eq 1 ]; then
			ln -srf /var/log/$TSN_APP /var/log/$log_file
		fi
	fi

	tsn_pid=$(cat $pid_file)

	if [ $SET_AFFINITY -ne 0 ]; then
		echo "setting PTP cpu affinity for pid $tsn_pid"
		if ! taskset -p "$CPU_MASK" "$tsn_pid";
		then
			echo "!!! WARNING: Setting PTP cpu affinity failed!"
		fi
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

start_tsn_stack()
{
	echo "Starting tsn endpoint stack"

	gptp_ipc_nodes
	mac_service_ipc_nodes
	srp_ipc_nodes

	if [ "$CFG_TSN_MULTI_ENDPOINTS" -eq 1 ]; then
		echo "		endpoint 0"
		start_tsn_endpoint_process 0

		echo "		endpoint 1"
		start_tsn_endpoint_process 1
	else
		start_tsn_endpoint_process 0
	fi
}

start_tsn_stack_br()
{
	echo "Starting tsn bridge stack"

	gptp_bridge_ipc_nodes
	mac_service_bridge_ipc_nodes
	srp_bridge_ipc_nodes

	if [ -f $PIDFILE_BR ]; then
		ps_list=$(${PS})
		tsn_running=$(echo "$ps_list" | grep "$TSN_APP.*-b")
		if [ -z "${tsn_running}" ]; then
			rm $PIDFILE_BR
		else
			return
		fi
	fi

	# Truncate log files to 0 if they exist, otherwise create them.
	# And use append mode with tee and redirection below to assure working log rotation.
	: > /var/log/$TSN_APP_BR
	: > /var/log/$FGPTP_APP_BR

	# Backward compatibility: redirect the srp-filtered log to /var/log/fgptp-br
	($TSN_APP -b -f "$PTP_CFG_FILE_BR" 2>&1 & echo $! > $PIDFILE_BR) | tee -a /var/log/$TSN_APP_BR | stdbuf -oL grep -v srp >> /var/log/$FGPTP_APP_BR &

	tsn_br_pid=$(cat $PIDFILE_BR)

	if [ $SET_AFFINITY -ne 0 ]; then
		echo "setting PTP bridge cpu affinity for pid $tsn_br_pid"
		if ! taskset -p "$CPU_MASK" "$tsn_br_pid";
		then
			echo "!!! WARNING: Setting PTP bridge cpu affinity failed!"
		fi
	fi
}

kill_process_pidfile()
{
	if [ -f "${1}" ]; then
		kill "$(cat "${1}")" > /dev/null 2>&1
		rm "${1}"
	fi
}

stop_tsn_stack()
{
	echo "Stopping tsn endpoint stack"

	if [ "$CFG_TSN_MULTI_ENDPOINTS" -eq 1 ]; then
		echo "		endpoint 0"
		kill_process_pidfile $PIDFILE_0

		echo "		endpoint 1"
		kill_process_pidfile $PIDFILE_1
	else
		kill_process_pidfile $PIDFILE_0
	fi
}

stop_tsn_stack_br()
{
	echo "Stopping tsn bridge stack"

	kill_process_pidfile $PIDFILE_BR
}

start_br()
{
	load_genavb_module

	if [ $PACKAGE = "BRIDGE" ] || [ $PACKAGE = "HYBRID" ]; then
		start_tsn_stack_br
	fi
}

stop_br()
{
	if [ $PACKAGE = "BRIDGE" ] || [ $PACKAGE = "HYBRID" ]; then
		stop_tsn_stack_br
	fi
}

start_ep()
{
	load_genavb_module

	if [ $PACKAGE = "ENDPOINT" ] || [ $PACKAGE = "HYBRID" ] || [ $PACKAGE = "ENDPOINT_GPTP" ]; then
		start_tsn_stack
	fi
}

stop_ep()
{
	if [ $PACKAGE = "ENDPOINT" ] || [ $PACKAGE = "HYBRID" ] || [ $PACKAGE = "ENDPOINT_GPTP" ]; then
		stop_tsn_stack
	fi
}

start()
{
	load_genavb_module

	if [ $PACKAGE = "BRIDGE" ] || [ $PACKAGE = "HYBRID" ]; then
		start_tsn_stack_br
	fi

	if [ $PACKAGE = "ENDPOINT" ] || [ $PACKAGE = "HYBRID" ] || [ $PACKAGE = "ENDPOINT_GPTP" ]; then
		start_tsn_stack
	fi
}

stop()
{
	if [ $PACKAGE = "BRIDGE" ] || [ $PACKAGE = "HYBRID" ]; then
		stop_tsn_stack_br
	fi

	if [ $PACKAGE = "ENDPOINT" ] || [ $PACKAGE = "HYBRID" ] || [ $PACKAGE = "ENDPOINT_GPTP" ]; then
		stop_tsn_stack
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
	stop_tsn_stack; stop_tsn_stack_br ; sleep 1; start_tsn_stack_br; start_tsn_stack
	;;
*)
	echo "Usage: $0 start|stop|restart|start_ep|start_br|stop_ep|stop_br" >&2
	exit 3
	;;
esac
