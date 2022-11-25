#!/bin/sh

BR_PORTS="swp0 swp1 swp2 swp3"
BR_ITF="br0"
NET_ITF="eno2"
PCP_TO_QOS_MAP=([0]="1" [1]="0" [2]="6" [3]="7" [4]="2" [5]="3" [6]="4" [7]="5")
SRP_VID=2

setup_forwarding()
{
	echo
	echo "# Setup bridge forwarding..."
	echo

	ip link set dev $NET_ITF up
	ip link add name $BR_ITF type bridge
	ip link set $BR_ITF up
	for port in $BR_PORTS; do
		ip link set master $BR_ITF $port up
	done
}


setup_pcp_mapping()
{
	echo
	echo "# Establish the PCP to QoS mapping for every port on the bridge..."
	echo

	for port in $BR_PORTS; do
		for (( pcp=0; pcp < 8; ++pcp )); do
			tsntool pcpmap -d $port -p $pcp -e 0 -c ${PCP_TO_QOS_MAP[$pcp]} -l 0
			tsntool pcpmap -d $port -p $pcp -e 1 -c ${PCP_TO_QOS_MAP[$pcp]} -l 1
		done;
	done
}


setup_shapers()
{
	echo
	echo "# Configure the qdiscs and shapers, with the correct handles, for every external port..."
	echo

	for port in $BR_PORTS; do
		tc qdisc add dev $port root handle 100: mqprio num_tc 8 map ${PCP_TO_QOS_MAP[@]} queues 1@0 1@1 1@2 1@3 1@4 1@5 1@6 1@7 hw 0
		tc qdisc replace dev $port handle 0x9007 parent 100:8 cbs locredit -2147483646 hicredit 2147483647 sendslope -1000000 idleslope 0 offload 0
		tc qdisc replace dev $port handle 0x9006 parent 100:7 cbs locredit -2147483646 hicredit 2147483647 sendslope -1000000 idleslope 0 offload 0
	done
}


setup_skb_priorities()
{
	echo
	echo "# Configure skb priorities..."
	echo

	for port in $BR_PORTS; do
		tc qdisc add dev $port clsact
		tc filter add dev $port egress basic match 'meta(priority eq 2)' or 'meta(priority eq 3)' action skbedit priority 0
	done
}


set_vlan_filtering()
{
	echo
	echo "# Enable Vlan filtering, set the correct Vlan IDs..."
	echo

	ip link set $BR_ITF type bridge vlan_filtering 1
	for port in $BR_PORTS; do
		bridge vlan add dev $port vid $SRP_VID master
	done

}


disable_multicast_flooding()
{
	echo
	echo "# Disable multicast flooding..."
	echo

	for port in $BR_PORTS; do
		bridge link set dev $port mcast_flood off
	done
}

add_default_mdb_entries()
{
	echo
	echo "# Add mdb entries for AVDECC and MAAP..."
	echo

	for port in $BR_PORTS; do
		bridge mdb add dev $BR_ITF port $port grp 91:e0:f0:01:00:00 permanent
		bridge mdb add dev $BR_ITF port $port grp 91:e0:f0:00:ff:00 permanent
	done
}


echo "AVB Bridge configuration for LS1028 RealTime Edge"

setup_forwarding

sleep 3

setup_pcp_mapping

setup_shapers

setup_skb_priorities

set_vlan_filtering

disable_multicast_flooding

add_default_mdb_entries
