#! /bin/sh

if [ $# -lt 2 ]; then
	echo "`basename $0` <interface> <period in µs> [\"iodevice\"]"
	exit
fi

ITF=$1
PERIOD=$2 # period in µs
TSN_GATE=4000 # gate window in ns

if [ "$3" = "iodevice" ]; then
	BT_MULT=750
else
	BT_MULT=250
fi

tc qdisc del dev $ITF root

tc qdisc replace dev $ITF root taprio \
num_tc 3 \
map 0 0 0 0 0 1 2 0 0 0 0 0 0 0 0 0 \
queues 1@0 1@1 1@2 \
base-time $((PERIOD*BT_MULT)) \
sched-entry S 0x2 $TSN_GATE \
sched-entry S 0x5 $((PERIOD*1000 - TSN_GATE)) \
flags 0x2

