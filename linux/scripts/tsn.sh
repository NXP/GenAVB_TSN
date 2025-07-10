#! /bin/sh

# Source common genavb functions
. "/etc/genavb/common_func.sh"

. "/etc/genavb/config_common"

. "/etc/genavb/config"

. "$GENAVB_TSN_CFG_FILE"

TSN_APP="tsn"
TSN_APP_BR="tsn-br"
PTP_CFG_FILE_0=/etc/genavb/fgptp.cfg
PTP_CFG_FILE_1=/etc/genavb/fgptp.cfg
PTP_CFG_FILE_BR=/etc/genavb/fgptp-br.cfg
PIDFILE_0=/var/run/"$TSN_APP"_0.pid
PIDFILE_1=/var/run/"$TSN_APP"_1.pid
PIDFILE_BR=/var/run/$TSN_APP_BR.pid
PHC2SYS_PIDFILE=/var/run/phc2sys.pid
# Backward compatibility
FGPTP_APP="fgptp"
FGPTP_APP_BR="fgptp-br"
GENAVB_SYSTEM_CONFIG_FILE=/etc/genavb/system.cfg
TSN_APP_HYBRID_OPTS=""

if [ -z "$AVB_MODE" ]; then
	AVB_MODE=0
fi

ITF_OVERRIDE=""
PTP_DEVICE_OVERRIDE=""
BR_PTP_OVERRIDE=""
BR_PORTS_OVERRIDE=""
TSN_ENDPOINT_NUM_TC_OVERRIDE=""

if [ "$AVB_MODE" -eq 1 ]; then
	NET_AVB_MODULE=1
else
	NET_AVB_MODULE=0
fi

if [ "$PACKAGE" = "ENDPOINT" ] || [ "$PACKAGE" = "HYBRID" ]; then
	SET_AFFINITY=1
else
	SET_AFFINITY=0
fi

set_machine_variables()
{
# Some platform-dependant variables:
# Default system.cfg for different usecases (Endpoint TSN, Endpoint AVB, Bridge ...)
# have the following assumptions:
# - Endpoint AVB: uses eth0 and /dev/ptp0
# - Endpoint TSN:
#     * uses eth1 and /dev/ptp1
#     * uses AF_XDP sockets for TSN traffic, on the following HW queues: TX Queue 2 and RX Queue 2
# - Bridge: uses ports swp0-swp3 and /dev/ptp1
#
# User may either:
# - set them manually in /etc/genavb/system.cfg.<usecase> AND set CFG_SYSTEM_CFG_NO_OVERRIDE to 1 in /etc/genavb/config
# - Put the right value in this function and let this script handle the override.
case $1 in
'imx8mpevk'|'imx93evk')
	TSN_ENDPOINT_NUM_TC_OVERRIDE=${ENETQOS_TSN_ENDPOINT_NUM_TC}
	;;
'imx8dxlevk')
	# i.MX8DXL EVK have two network controllers but only one can be enabled at runtime
	# So both Enet Qos (TSN) and Enet (AVB) point to eth0
	if [ "$AVB_MODE" -eq 0 ] && [ "$PACKAGE" = "ENDPOINT" ]; then
		ITF_OVERRIDE=eth0
		PTP_DEVICE_OVERRIDE=/dev/ptp0
	fi

	TSN_ENDPOINT_NUM_TC_OVERRIDE=${ENETQOS_TSN_ENDPOINT_NUM_TC}
	;;
'imx93qsb')
	# i.MX93 9x9 Quick Start Board has a single network interface connected to Enet Qos.
	if [ "$AVB_MODE" -eq 0 ] && [ "$PACKAGE" = "ENDPOINT" ]; then
		ITF_OVERRIDE=eth0
		PTP_DEVICE_OVERRIDE=/dev/ptp0
	fi

	TSN_ENDPOINT_NUM_TC_OVERRIDE=${ENETQOS_TSN_ENDPOINT_NUM_TC}
	;;
'imx93evkauto')
	# For i.MX93 14X14 EVK board Bridge/Hybrid (with SJA1105), override the system configuration based on configured bridge ports.
	# Default configuration in set on interface swp[0-3], while i.MX93EVK AUTO with SJA1105 uses swp[1-4] as bridge ports.
	BR_PORTS_OVERRIDE="swp1, swp2, swp3, swp4"

	if [ "$PACKAGE" = "HYBRID" ]; then
		# Using external link to connect endpoint (ENET2) to bridge (ENET1):
		# Use hybrid mode with gPTP sync
		TSN_APP_HYBRID_OPTS="-H 2"

		# i.MX93 14x14 EVK uses fec interface for Hybrid AVB
		ITF_OVERRIDE="$(get_network_interface "fec")"
		PTP_DEVICE_OVERRIDE="$(get_ptp_device "fec")"
		BR_PTP_OVERRIDE="$(get_ptp_device "sja1105")"
		if [ -z "$ITF_OVERRIDE" ] || [ -z "$PTP_DEVICE_OVERRIDE" ] || [ -z "$BR_PTP_OVERRIDE" ]; then
			echo "Failed to retrieve interface using the fec driver and the corresponding ptp devices on Hybrid config"
			exit 1
		fi

	elif [ "$PACKAGE" = "ENDPOINT" ]; then
		if [ "$AVB_MODE" -eq 1 ]; then
			# i.MX93 14x14 EVK uses fec interface and /dev/ptp0 for Endpoint AVB
			ITF_OVERRIDE="$(get_network_interface "fec")"
			PTP_DEVICE_OVERRIDE="$(get_ptp_device "fec")"
		else
			# i.MX93 14x14 EVK uses imx-dwmac interface and /dev/ptp1 for Endpoint TSN
			ITF_OVERRIDE="$(get_network_interface "imx-dwmac")"
			PTP_DEVICE_OVERRIDE="$(get_ptp_device "imx-dwmac")"
		fi

		if [ -z "$ITF_OVERRIDE" ] || [ -z "$PTP_DEVICE_OVERRIDE" ]; then
			echo "Failed to retrieve interface using the right driver (AVB: fec, TSN: imx-dwmac) and the corresponding ptp device on Endpoint config"
			exit 1
		fi
	fi

	TSN_ENDPOINT_NUM_TC_OVERRIDE=${ENETQOS_TSN_ENDPOINT_NUM_TC}
	;;
'imx95evk')
	# i.MX95 19x19 EVK uses eth0 and /dev/ptp0 for Endpoint TSN
	if [ "$AVB_MODE" -eq 0 ] && [ "$PACKAGE" = "ENDPOINT" ]; then
		ITF_OVERRIDE=eth0
		PTP_DEVICE_OVERRIDE=/dev/ptp0
	fi

	TSN_ENDPOINT_NUM_TC_OVERRIDE=${IMX95EVK_TSN_ENDPOINT_NUM_TC}
	;;
'imx943evk')
	# i.MX943 19x19 EVK uses eth2 and /dev/ptp1 for Endpoint TSN
	if [ "$AVB_MODE" -eq 0 ] && [ "$PACKAGE" = "ENDPOINT" ]; then
		ITF_OVERRIDE=eth2
		PTP_DEVICE_OVERRIDE=/dev/ptp1
	fi

	TSN_ENDPOINT_NUM_TC_OVERRIDE=${IMX943EVK_TSN_ENDPOINT_NUM_TC}
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

srp_hybrid_ipc_nodes()
{
	major=$(grep ipcdrv /proc/devices | awk '{ print $1 }' -)

	rm -fr /dev/ipc_srp_bridge_endpoint_rx
	rm -fr /dev/ipc_srp_bridge_endpoint_tx
	rm -fr /dev/ipc_srp_endpoint_bridge_rx
	rm -fr /dev/ipc_srp_endpoint_bridge_tx

	mknod /dev/ipc_srp_bridge_endpoint_rx c "$major" 202
	mknod /dev/ipc_srp_bridge_endpoint_tx c "$major" 203
	mknod /dev/ipc_srp_endpoint_bridge_rx c "$major" 204
	mknod /dev/ipc_srp_endpoint_bridge_tx c "$major" 205

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

load_genavb_modules()
{
	ipc_loaded=$(lsmod |grep genavbtsn_ipc)
	if [ -z "${ipc_loaded}" ]; then
		if ! insmod /lib/modules/"$(uname -r)"/genavb/genavbtsn_ipc.ko > /dev/null 2>&1; then
			echo "Failed to load genavbtsn_ipc kernel module"
			exit 1
		fi
	fi

	if [ $NET_AVB_MODULE -eq 1  ]; then
		net_avb_loaded=$(lsmod |grep genavbtsn_net_avb)
		if [ -z "${net_avb_loaded}" ]; then
			if ! insmod /lib/modules/"$(uname -r)"/genavb/genavbtsn_net_avb.ko > /dev/null 2>&1; then
				echo "Failed to load genavbtsn_net_avb kernel module"
				exit 1
			fi
		fi

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

set_system_cfg_file()
{
	if [ "$PACKAGE" = "BRIDGE" ]; then
		if [ ! -e ${GENAVB_SYSTEM_CONFIG_FILE}.bridge ]; then
			echo "Failed to set system.cfg: ${GENAVB_SYSTEM_CONFIG_FILE}.bridge not found"
			exit 1
		fi

		ln -sf "${GENAVB_SYSTEM_CONFIG_FILE}.bridge" "${GENAVB_SYSTEM_CONFIG_FILE}"
	elif [ "$PACKAGE" = "HYBRID" ]; then
		if [ ! -e ${GENAVB_SYSTEM_CONFIG_FILE}.hybrid_avb ]; then
			echo "Failed to set system.cfg: ${GENAVB_SYSTEM_CONFIG_FILE}.hybrid_avb not found"
			exit 1
		fi

		ln -sf "${GENAVB_SYSTEM_CONFIG_FILE}.hybrid_avb" "${GENAVB_SYSTEM_CONFIG_FILE}"
	else
		if [ "$AVB_MODE" -eq 0 ]; then
			if [ ! -e ${GENAVB_SYSTEM_CONFIG_FILE}.tsn ]; then
				echo "Failed to set system.cfg: ${GENAVB_SYSTEM_CONFIG_FILE}.tsn not found"
				exit 1
			fi

			ln -sf "${GENAVB_SYSTEM_CONFIG_FILE}.tsn" "${GENAVB_SYSTEM_CONFIG_FILE}"
		else
			if [ ! -e ${GENAVB_SYSTEM_CONFIG_FILE}.avb ]; then
				echo "Failed to set system.cfg: ${GENAVB_SYSTEM_CONFIG_FILE}.avb not found"
				exit 1
			fi

			ln -sf "${GENAVB_SYSTEM_CONFIG_FILE}.avb" "${GENAVB_SYSTEM_CONFIG_FILE}"
		fi
	fi

	system_cfg_filename=$(readlink -f ${GENAVB_SYSTEM_CONFIG_FILE})

	echo "Using file '${system_cfg_filename}' as system configuration file"

	if [ "$CFG_SYSTEM_CFG_NO_OVERRIDE" -eq 0 ]; then
		if [ -n "$ITF_OVERRIDE" ]; then
			# Override the default system configuration endpoint interface.
			echo "Override '${system_cfg_filename}': Use interface '$ITF_OVERRIDE'"
			sed -E "/^[ \t]*endpoint./s|eth[0-9]+|${ITF_OVERRIDE}|g" -i "${system_cfg_filename}"
		fi

		if [ -n "$PTP_DEVICE_OVERRIDE" ]; then
			# Override the default system configuration endpoint PTP device.
			echo "Override '${system_cfg_filename}': Use PTP device '$PTP_DEVICE_OVERRIDE'"
			sed -E "/^[ \t]*endpoint./s|/dev/ptp[0-9]+|${PTP_DEVICE_OVERRIDE}|g" -i "${system_cfg_filename}"
		fi

		if [ -n "$BR_PTP_OVERRIDE" ]; then
			# Override the default system configuration bridge PTP device.
			echo "Override '${system_cfg_filename}': Use BR PTP device '$BR_PTP_OVERRIDE'"
			sed -E "/^[ \t]*bridge./s|/dev/ptp[0-9]+|${BR_PTP_OVERRIDE}|g" -i "${system_cfg_filename}"
		fi

		if [ -n "$TSN_ENDPOINT_NUM_TC_OVERRIDE" ]; then
			# For Endpoint TSN, override the system configuration for AF XDP queues based on configured number of traffic classes.
			if [ "$TSN_ENDPOINT_NUM_TC_OVERRIDE" -eq 4 ]; then
				PCP_TO_QOS_MAP="${PCP_TO_QOS_MAP_4TC_0SR}"
			elif [ "$TSN_ENDPOINT_NUM_TC_OVERRIDE" -eq 5 ]; then
				PCP_TO_QOS_MAP="${PCP_TO_QOS_MAP_5TC_0SR}"
			elif [ "$TSN_ENDPOINT_NUM_TC_OVERRIDE" -eq 6 ]; then
				PCP_TO_QOS_MAP="${PCP_TO_QOS_MAP_6TC_0SR}"
			else
				echo "Error: Unspported number of traffic classes ($TSN_ENDPOINT_NUM_TC_OVERRIDE) for TSN endpoint"
				exit 1
			fi

			TSN_TRAFFIC_TC="$(get_list_index_val "$PCP_TO_QOS_MAP" "$TSN_TRAFFIC_PRIO")"

			echo "Override '${system_cfg_filename}': For Endpoint TSN, use AF_XDP TX and RX HW queues ($TSN_TRAFFIC_TC)'"
			sed -e "/^endpoint_queue_[r|t]x /s|=.*[0-9]\+|= $TSN_TRAFFIC_TC, $TSN_TRAFFIC_TC|g" -i "${system_cfg_filename}"
		fi

		if [ -n "$BR_PORTS_OVERRIDE" ]; then
			# Override the default system configuration bridge ports.
			echo "Override '${system_cfg_filename}': For AVB Bridge/Hybrid, use ports '$BR_PORTS_OVERRIDE'"
			sed -e "s|.*bridge_0.*|bridge_0 = ${BR_PORTS_OVERRIDE}|g" -i "${system_cfg_filename}"
		fi
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
		$TSN_APP -f "$gptp_conf_file" -e $endpoint_number "$TSN_APP_HYBRID_OPTS" > /var/log/$log_file 2>&1 &

		tsn_pid=$!

		# save process pid
		echo $tsn_pid > $pid_file
	else
		# Truncate log files to 0 if they exist, otherwise create them.
		# And use append mode with tee and redirection below to assure working log rotation.
		: > /var/log/$TSN_APP
		: > /var/log/$FGPTP_APP

		# Backward compatibility: For endpoint 0, redirect the srp-filtered log to /var/log/fgptp
		($TSN_APP -f "$gptp_conf_file" -e $endpoint_number "$TSN_APP_HYBRID_OPTS" 2>&1 & echo $! > "$pid_file") | tee -a /var/log/$TSN_APP | stdbuf -oL grep -v srp >> /var/log/$FGPTP_APP &

		# wait for above subshell to create the pid file
		wait_file_exists "$pid_file" "5"

		tsn_pid=$(cat $pid_file)

		if [ "$CFG_TSN_MULTI_ENDPOINTS" -eq 1 ]; then
			ln -srf /var/log/$TSN_APP /var/log/$log_file
		fi
	fi

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
			PHC_DEVICE=$(get_system_config_phc_device "${system_cfg_filename}" "$AVB_MODE")
			echo "Starting phc2sys on ${PHC_DEVICE}"
			taskset "$CPU_MASK" phc2sys -s "${PHC_DEVICE}" -O 0 -S 0.00002 &
			echo $! > $PHC2SYS_PIDFILE
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
	($TSN_APP -b -f "$PTP_CFG_FILE_BR" "$TSN_APP_HYBRID_OPTS" 2>&1 & echo $! > $PIDFILE_BR) | tee -a /var/log/$TSN_APP_BR | stdbuf -oL grep -v srp >> /var/log/$FGPTP_APP_BR &

	# wait for above subshell to create the pid file
	wait_file_exists "$PIDFILE_BR" "5"

	tsn_br_pid=$(cat $PIDFILE_BR)

	if [ $SET_AFFINITY -ne 0 ]; then
		echo "setting PTP bridge cpu affinity for pid $tsn_br_pid"
		if ! taskset -p "$CPU_MASK" "$tsn_br_pid";
		then
			echo "!!! WARNING: Setting PTP bridge cpu affinity failed!"
		fi
	fi
}

stop_tsn_stack()
{
	echo "Stopping tsn endpoint stack"

	kill_process_pidfile $PHC2SYS_PIDFILE

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
	load_genavb_modules

	if [ "$PACKAGE" = "BRIDGE" ] || [ "$PACKAGE" = "HYBRID" ]; then
		start_tsn_stack_br
	fi
}

stop_br()
{
	if [ "$PACKAGE" = "BRIDGE" ] || [ "$PACKAGE" = "HYBRID" ]; then
		stop_tsn_stack_br
	fi
}

start_ep()
{
	load_genavb_modules
	set_system_cfg_file

	if [ "$PACKAGE" = "ENDPOINT" ] || [ "$PACKAGE" = "HYBRID" ]; then
		start_tsn_stack
	fi
}

stop_ep()
{
	if [ "$PACKAGE" = "ENDPOINT" ] || [ "$PACKAGE" = "HYBRID" ]; then
		stop_tsn_stack
	fi
}

start()
{
	load_genavb_modules
	set_system_cfg_file

	# Create SRP hybrid device nodes before launching endpoint or bridge stack
	if [ "$PACKAGE" = "HYBRID" ]; then
		srp_hybrid_ipc_nodes
	fi

	if [ "$PACKAGE" = "BRIDGE" ] || [ "$PACKAGE" = "HYBRID" ]; then
		start_tsn_stack_br
	fi

	if [ "$PACKAGE" = "ENDPOINT" ] || [ "$PACKAGE" = "HYBRID" ]; then
		start_tsn_stack
	fi
}

stop()
{
	if [ "$PACKAGE" = "BRIDGE" ] || [ "$PACKAGE" = "HYBRID" ]; then
		stop_tsn_stack_br
	fi

	if [ "$PACKAGE" = "ENDPOINT" ] || [ "$PACKAGE" = "HYBRID" ]; then
		stop_tsn_stack
	fi
}

############################ MAIN ############################

PATH=/sbin:/usr/sbin:/bin:/usr/bin:.

NB_CPU=$(grep -c processor /proc/cpuinfo)

if  [ "$NB_CPU" -gt 2 ];then
	[ $AVB_MODE -eq 1 ] && CPU_CORE="${AVB_MODE_TSN_STACK_CORE_GT2C}" || CPU_CORE="${TSN_MODE_TSN_STACK_CORE_GT2C}"
elif  [ "$NB_CPU" -eq 2 ];then
	[ $AVB_MODE -eq 1 ] && CPU_CORE="${AVB_MODE_TSN_STACK_CORE_2C}" || CPU_CORE="${TSN_MODE_TSN_STACK_CORE_2C}"
else
	[ $AVB_MODE -eq 1 ] && CPU_CORE="${AVB_MODE_TSN_STACK_CORE_1C}" || CPU_CORE="${TSN_MODE_TSN_STACK_CORE_1C}"
fi

CPU_MASK="$(( 1 << CPU_CORE ))"

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
	stop; sleep 1; start;
	;;
*)
	echo "Usage: $0 start|stop|restart|start_ep|start_br|stop_ep|stop_br" >&2
	exit 3
	;;
esac
