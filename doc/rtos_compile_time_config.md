# Compile Time Configuration {#compile_configuration}

## Overview

The GenAVB/TSN stack, on RTOS platforms, supports extensive compile time configuration for the low level OS Abstraction Layer.
The configuration is provided by the application, through board specific header files.

## Configuration File

The configuration top level file is named genavb_sdk.h and must be found in the include path.

For example, for i.MX RT1180 device, cm33 core:
```
freertos_avb_apps/evkmimxrt1180/demo_apps/avb_tsn/tsn_app/cm33/genavb_sdk.h
```
which includes:
```
freertos_avb_apps/devices/MIMXRT118x/common/cm33/genavb_sdk_common.h
freertos_avb_apps/evkmimxrt1180/demo_apps/avb_tsn/common/cm33/genavb_sdk_net_port.h
```

## Configuration Parameters

### Network Configuration

Generic network configuration.

#### BOARD_NET_RX_PACKETS
- **Type**: Integer
- **Description**: Number of packets processed per network receive task polling period
- **Example**: `2` (only process 2 packets on the given period)

#### BOARD_NET_RX_PERIOD_MULT
- **Type**: Integer
- **Description**: Multiplier for the network receive task polling period. Use to increase the default 125us polling period.
- **Example**: `2` (increase default polling period from 125us to 250us)

### Network Port Configuration

Configuration for each of the network ports present on the board.

#### BOARD_NUM_PORTS
- **Type**: Integer
- **Description**: Total number of network ports available on the board (internal/external, switch/endpoint)
- **Example**: `1` (single network port)

#### Port Instance Configuration

Each network port is configured using a set of macros with the pattern `BOARD_NET_PORT<n>_*` where `<n>` is the port index.
The port index must always be in a continuous range, from 0 to `BOARD_NUM_PORTS - 1`. This configuration is used in `gen_avb/rtos/net_port.c`.

##### BOARD_NET_PORT<n>_DRV_TYPE
- **Type**: `net_driver_type_t` enum
- **Description**: Network driver type for the port
- **Possible Values**:
  - `ENET_t` - ENET Endpoint port (i.MX RT1050 and MCX E247)
  - `ENET_1G_t` - ENET 1G Endpoint port (i.MX RT1170 only)
  - `ENET_QOS_t` - ENET_QOS Endpoint port (i.MX RT1170 and MCX E31B)
  - `ENETC_1G_t` - ENETC Endpoint port (i.MX RT1180 only)
  - `ENETC_PSEUDO_1G_t` - ENETC Pseudo Endpoint port (i.MX RT1180 only)
  - `NETC_SW_t` - NETC Switch port (i.MX RT1180 only)
- **Example**: `ENETC_PSEUDO_1G_t`

##### BOARD_NET_PORT<n>_DRV_INDEX
- **Type**: Integer
- **Description**: Driver instance index. Specific to each driver type
- **Example**: `0`

##### BOARD_NET_PORT<n>_DRV_BASE
- **Type**: Hardware base identifier (driver specific)
- **Description**: Hardware base address or identifier for the network controller
- **Example**: `kNETC_ENETC1VSI1` (ENETC1 Virtual Station Interface 1)

##### BOARD_NET_PORT<n>_PHY_INDEX
- **Type**: Integer
- **Description**: PHY (Physical Layer) device index. Points to one of the `BOARD_PHY<n>_*` entries.
- **Values**:
  - `-1` - No PHY (e.g., for virtual interfaces)
  - `>= 0` - PHY device index
- **Example**: `-1`

##### BOARD_NET_PORT<n>_MII_MODE
- **Type**: MII mode enum (driver specific)
- **Description**: Media Independent Interface mode
- **Possible Values**:
  - ENET_t and ENET_1G_t
    - `kENET_MiiMode` - MII mode
    - `kENET_RmiiMode` - RMII mode
    - `kENET_RgmiiMode` - RGMII mode
  - ENET_QOS_t
    - `kENET_QOS_MiiMode` - MII mode
    - `kENET_QOS_RmiiMode` - RMII mode
    - `kENET_QOS_RgmiiMode` - RGMII mode
  - NETC_SW_t and ENETC_1G_t ports
    - `kNETC_MiiMode` - MII mode
    - `kNETC_RmiiMode` - RMII mode
    - `kNETC_RgmiiMode` - RGMII mode
- **Example**: `kNETC_RgmiiMode`

##### BOARD_NET_PORT<n>_HW_CLOCK
- **Type**: Integer
- **Description**: Hardware clock identifier for timestamping
- **Example**: `0`

##### BOARD_NET_PORT<n>_TRAFFIC_CLASS_MAX
- **Type**: Integer
- **Description**: Maximum number of traffic classes (queues) supported
- **Range**: Typically 1-8
- **Example**: `4`

##### BOARD_NET_PORT<n>_SR_CLASS_MAX
- **Type**: Integer
- **Description**: Maximum number of Stream Reservation (SR) classes
- **Example**: `0` (no SR classes)

### MDIO Configuration

Configuration for each of the MDIO bus present on the board, providing connection between network controllers and phy devices.

#### BOARD_NUM_MDIO
- **Type**: Integer
- **Description**: Number of MDIO (Management Data Input/Output) interfaces
- **Example**: `1`

#### MDIO Instance Configuration

Each mdio bus is configured using a set of macros with the pattern `BOARD_MDIO<n>_*` where `<n>` is the mdio bus index.
The mdio bus index must always be in a continuous range, from 0 to `BOARD_NUM_MDIO - 1`.

##### BOARD_MDIO<n>_DRV_TYPE
- **Type**: Integer
- **Description**: MDIO driver type for the mdio bus
- **Possible Values**:
  - `ENET_t` - ENET MDIO bus (i.MX RT1050 and MCX E247)
  - `ENET_1G_t` - ENET 1G MDIO bus (i.MX RT1170 only)
  - `ENET_QOS_t` - ENET_QOS MDIO bus (i.MX RT1170 and MCX E31B)
  - `NETC_EMDIO_t` - NETC EMDIO bus (i.MX RT1180 only)
  - `NETC_PORT_EMDIO_t` - NETC Port EMDIO bus (i.MX RT1180 only)
- **Example**: `ENET_QOS_t`

##### BOARD_MDIO<n>_DRV_INDEX
- **Type**: Integer
- **Description**: Driver instance index.
- **Example**: `0`

### PHY Configuration

Configuration for each of the PHY devices present on the board.

#### BOARD_NUM_PHY
- **Type**: Integer
- **Description**: Number of PHY devices
- **Example**: `0`

#### PHY Instance Configuration

Each phy is configured using a set of macros with the pattern `BOARD_PHY<n>_*` where `<n>` is the phy index.
The phy index must always be in a continuous range, from 0 to `BOARD_NUM_PHY - 1`.

##### BOARD_PHY<n>_MDIO_ID
- **Type**: Integer
- **Description**: MDIO bus index, to which the PHY is attached. Points to one of the `BOARD_MDIO<n>_*` entries.
- **Example**: `0`

##### BOARD_PHY<n>_ADDR
- **Type**: Integer
- **Description**: PHY Address in the MDIO bus.
- **Example**: `0`

##### BOARD_PHY<n>_OPS
- **Type**: `phy_operations_t` PHY operations structure
- **Description**: MCUX SDK PHY driver specific operations
- **Example**: `phyksz8081_ops`

##### BOARD_PHY<n>_RX_LATENCY_100M
- **Type**: Integer
- **Description**: PHY receive latency when in 100Mbps mode, in ns. Used for receive timestamping compensation.
- **Example**: `800`

##### BOARD_PHY<n>_TX_LATENCY_100M
- **Type**: Integer
- **Description**: PHY transmit latency when in 100Mbps mode, in ns. Used for transmit timestamping compensation.
- **Example**: `800`

##### BOARD_PHY<n>_RX_LATENCY_1G
- **Type**: Integer
- **Description**: PHY receive latency when in 1Gbps mode, in ns. Used for receive timestamping compensation.
- **Example**: `300`

##### BOARD_PHY<n>_TX_LATENCY_1G
- **Type**: Integer
- **Description**: PHY receive latency when in 1Gpbs mode, in ns. Used for transmit timestamping compensation.
- **Example**: `300`

### Network Port Driver Configuration

#### ENET/ENET_1G

##### BOARD_NUM_ENET_PORTS
- **Type**: Integer
- **Description**: Number of ENET ports
- **Example**: `1`

#### ENET_QOS

##### BOARD_NUM_ENET_QOS_PORTS
- **Type**: Integer
- **Description**: Number of ENET_QOS ports
- **Example**: `1`

#### ENETC

##### BOARD_NUM_ENETC_PORTS
- **Type**: Integer
- **Description**: Number of ENETC ports
- **Example**: `1`

##### BOARD_ENETC<n>_RX_ZERO_COPY
- **Type**: Preprocessor define
- **Description**: Enable zero-copy receive for ENETC instance.
- **Usage**: Define this macro to enable the feature

#### NETC Switch

##### BOARD_NUM_NETC_SWITCHES
- **Type**: Integer
- **Description**: Number of NETC switch instances
- **Example**: `0` (no switch)

##### BOARD_NUM_NETC_PORTS
- **Type**: Integer
- **Description**: Number of NETC switch ports on the SoC (always 5 for i.MX RT1180).
- **Example**: `5`.


### Hardware Timer Configuration

#### GPT

##### BOARD_NUM_GPT_TIMERS
- **Type**: Integer
- **Description**: Number of GPT (General Purpose Timer) instances
- **Example**: `1`

##### GPT Instance Configuration

Each GPT is configured using a set of macros with the pattern `BOARD_GPT_<n>_*` where `<n>` is the GPT index.
The GPT index must always be in a continuous range, from 0 to `BOARD_NUM_GPT_TIMERS - 1`.

###### BOARD_GPT_<n>_BASE
- **Type**: Hardware base address
- **Description**: Base address of the GPT instance
- **Example**: `GPT1`

###### BOARD_GPT_<n>_IRQ
- **Type**: Integer
- **Description**: IRQ identifier for the GPT instance
- **Example**: `GPT1_IRQn`

###### BOARD_GPT_<n>_IRQ_HANDLER
- **Type**: Function name
- **Description**: IRQ handler function for the GPT instance
- **Example**: `GPT1_IRQHandler`

###### BOARD_GPT_<n>_REC_BASE
- **Type**: Integer
- **Description**: Recovery time for the GPT instance
- **Example**: `GPT1`

#### FTM (MCX E247 only)

##### BOARD_NUM_FTM_TIMERS
- **Type**: Integer
- **Description**: Number of FTM (FlexTimer Module) instances
- **Example**: `1`

##### FTM Instance Configuration

Each FTM is configured using a set of macros with the pattern `BOARD_FTM_<n>_*` where `<n>` is the FTM index.
The FTM index must always be in a continuous range, from 0 to `BOARD_NUM_FTM_TIMERS - 1`.

###### BOARD_FTM_<n>_BASE
- **Type**: Hardware base address
- **Description**: Base address of the FTM instance
- **Example**: `FTM0`

###### BOARD_FTM_<n>_IRQ_ID
- **Type**: Integer
- **Description**: IRQ identifier for the FTM instance
- **Example**: `FTM0_IRQn`

###### BOARD_FTM_<n>_IRQ_HANDLER
- **Type**: Function name
- **Description**: IRQ handler function for the FTM instance
- **Example**: `FTM0_IRQHandler`

#### STM (MCX E31B only)

##### BOARD_NUM_STM_TIMERS
- **Type**: Integer
- **Description**: Number of STM (System Timer Module) instances.
- **Example**: `1`

##### STM Instance Configuration

Each STM is configured using a set of macros with the pattern `BOARD_STM_<n>_*` where `<n>` is the STM index.
The STM index must always be in a continuous range, from 0 to `BOARD_NUM_STM_TIMERS - 1`.

###### BOARD_STM_<n>_BASE
- **Type**: Hardware base address
- **Description**: Base address of the STM instance
- **Example**: `STM0`

###### BOARD_STM_<n>_IRQ
- **Type**: Integer
- **Description**: IRQ identifier for the STM instance
- **Example**: `STM0_IRQn`

###### BOARD_STM_<n>_IRQ_HANDLER
- **Type**: Function name
- **Description**: IRQ handler function for the STM instance
- **Example**: `STM0_IRQHandler`

### Hardware Clock Configuration

#### NETC

##### BOARD_NUM_NETC_HW_CLOCK
- **Type**: Integer
- **Description**: Number of NETC hardware clock instances (always 1 for i.MX RT1180).
- **Example**: `1`

##### NETC Hardware Clock Instance Configuration

Each NETC hardware clock is configured using a set of macros with the pattern `BOARD_NETC_HW_CLOCK_<n>_*` where `<n>` is the hardware clock index.
The hardware clock index must always be in a continuous range, from 0 to `BOARD_NUM_NETC_HW_CLOCK - 1`.

### Message Interrupt Configuration

#### BOARD_NUM_MSGINTR
- **Type**: Integer
- **Description**: Number of MSGINTR instances.
- **Example**: `1

##### MSGINTR Instance Configuration

Each MSGINTR is configured using a set of macros with the pattern `BOARD_MSGINTR<n>_*` where `<n>` is the MSGINTR index.
The MSGINTR index must always be in a continuous range, from 0 to `BOARD_NUM_MSGINTR - 1`.

##### BOARD_MSGINTR<n>_BASE
- **Type**: Hardware base address
- **Description**: Base address of the MSGINTR instance
- **Example**: `MSGINTR1`

