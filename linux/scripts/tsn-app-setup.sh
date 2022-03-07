#! /bin/sh

ITF_NAME="eth1"
ITF_IRQ=$(grep $ITF_NAME /proc/interrupts | awk -F: '{ print $1 }')

# Extract Qbv config from genavb config files
grep -q "PROFILE=2" /etc/genavb/config && IODEVICE="iodevice" || IODEVICE=""
PERIOD_US=`grep -o "\-p [0-9]\+" /etc/genavb/apps-tsn-network-controller.cfg |grep -o "[0-9]\+"`
[ "$PERIOD_US" = "" ] && PERIOD_US=2000 || PERIOD_US=$((PERIOD_US / 1000 ))

# Force otherwise unmoveable kernel threads
# (timers, etc) off CPU core 2.
echo 0 > /sys/devices/system/cpu/cpu2/online
echo 1 > /sys/devices/system/cpu/cpu2/online

#Enable interface and setup VLANs
ip link add link $ITF_NAME name vlan0 type vlan id 2
ip link set dev vlan0 up

# Load XDP program.
ip link set dev $ITF_NAME xdp obj /lib/firmware/genavb/genavb-xdp.bin

# Disable coalescing and pause frames.
ethtool -C $ITF_NAME rx-usecs 16 tx-usecs 10000 tx-frames 1
ethtool -A $ITF_NAME autoneg off rx off tx off

# Setup 802.1Qbv (using tc taprio qdisc).
tsn-app-taprio-config.sh $ITF_NAME $PERIOD_US $IODEVICE

# Setup Rx classification (using tc flower qdisc).
# Must be called after taprio_config (depends on
# traffic class definition from taprio).
modprobe cls_flower
tc qdisc add dev $ITF_NAME ingress
tc filter del dev $ITF_NAME ingress
tc filter add dev $ITF_NAME parent ffff: protocol 802.1Q flower vlan_prio 5 hw_tc 1

# Start gPTP daemon.
fgptp.sh start

# Make sure the link and XDP are ready before
# starting tsn-app.
sleep 6

# Start tsn-app.
avb.sh start

# Move processes off CPU core 2. Ignore errors
# for unmoveable processes.
for i in `ps aux | grep -v PID | awk '{print $2;}'`; do taskset -p b $i > /dev/null 2>&1; done

# Move workqueues off CPU core 2.
for i in `find /sys/devices/virtual/workqueue -name cpumask`; do echo b > $i; done

# Configure real-time priorities and CPU affinities.
# Enable threaded NAPI.
echo 1 > /sys/class/net/$ITF_NAME/threaded

# Move all best-effort/non-critical queues
# to CPU core 3 (4th core)
taskset -p 8 `pgrep napi/$ITF_NAME-rx-2`
taskset -p 8 `pgrep napi/$ITF_NAME-rx-3`
taskset -p 8 `pgrep napi/$ITF_NAME-rx-4`
taskset -p 8 `pgrep napi/$ITF_NAME-tx-0`
taskset -p 8 `pgrep napi/$ITF_NAME-zc-0`
taskset -p 8 `pgrep napi/$ITF_NAME-zc-2`
taskset -p 8 `pgrep napi/$ITF_NAME-zc-3`
taskset -p 8 `pgrep napi/$ITF_NAME-zc-4`

# Ethernet IRQ on CPU core 1 (2nd core)
# High prio to ensure high prio packets
# are handled quickly.
for irq in $ITF_IRQ; do
	taskset -p 2 `pgrep irq/$irq-$ITF_NAME`
	echo 2 > /proc/irq/$irq/smp_affinity
	chrt -pf 66 `pgrep irq/$irq-$ITF_NAME`
done

# gPTP queues on CPU core 1 (2nd core)
# real-time prio, lower than gPTP daemon
taskset -p 2 `pgrep napi/$ITF_NAME-rx-0`
taskset -p 2 `pgrep napi/$ITF_NAME-tx-2`
chrt -pf 1 `pgrep napi/$ITF_NAME-rx-0`
chrt -pf 1 `pgrep napi/$ITF_NAME-tx-2`

# TSN traffic queues on CPU core 2 (3rd core)
# same prio as tsn-app for AF_XDP queue
# in AF_PACKET mode, give network queues a 
# higher prio than tsn-app
taskset -p 4 `pgrep napi/$ITF_NAME-zc-1`
taskset -p 4 `pgrep napi/$ITF_NAME-rx-1`
taskset -p 4 `pgrep napi/$ITF_NAME-tx-1`
chrt -pf 60 `pgrep napi/$ITF_NAME-zc-1`
chrt -pf 61 `pgrep napi/$ITF_NAME-rx-1`
chrt -pf 61 `pgrep napi/$ITF_NAME-tx-1`
