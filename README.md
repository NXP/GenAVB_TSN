GenAVB/TSN
----------
GenAVB/TSN is a generic AVB/TSN stack developed by NXP for NXP MCUs and MPUs.
It is cross-platform, currently supporting Linux and FreeRTOS.


Supported hardware targets and configurations
---------------------------------------------
This project supports several hardware targets and several different
configuration modes. The table below summarizes the supported combinations,
overall and for this specific release:
- **O**: supported by the project and on this specific release
- x: supported by the project, but not on this release
- empty cell: unsupported combination

|                 | AVB endpoint | TSN endpoint | bridge | hybrid |
| :-------------: | :----------: | :----------: | :----: | :----: |
| linux i.MX6     |      x       |              |    x   |    x   |
| linux i.MX8     |      x       |      x       |    x   |    x   |
| linux LS1028    |              |              |    x   |        |
| freertos RT1052 |    **O**     |      x       |        |        |
| freertos RT1176 |    **O**     |    **O**     |        |        |


Features
--------
- IEEE-802.1AS-2020 implementation, both time-aware Bridge and Endpoint support.
- IEEE-802.1Qat-2010 implementation.
- Binary protocol stacks running in standalone userspace processes.
- Public API provided by binary library plus header files.
- Example applications.


Repository structure
--------------------
- api:      the public API
- apps:     source code and makefiles for example applications
- avdecc:   IEEE 1722.1-2013 component stack
- avtp:     IEEE 1722-2016 component stack
- common:   common code
- configs:  configuration files
- doc:      documentation
- gptp:     IEEE 802.1AS-2020 component stack
- freertos: FreeRTOS specific code
- linux:    Linux specific code
- maap:     MAAP component code
- public:   common code shared between applications and stack
- srp:      IEEE 802.1Qat-2010 component stack


Build
-----
GenAVB/TSN is using Cmake to generate build system.

Some preliminary configuration is required to provide path to the toolchain,
staging directory and external components.

Local configuration files can be included by the build system in order to
to define some variables specific to the developer environment.
The local config file name is `./local_config_${target}.cmake`


Build requirements
------------------
- Linux host system and development tools (git, make, doxygen for the docs, ...)
- An ARM gcc toolchain is sufficient for the stack. However building the linux
applications requires a more complete cross-compilation SDK (because of
additional dependencies such as alsa, gstreamer and qt5).
- AVB endpoint builds depend on custom changes to the Linux kernel and Yocto distribution: https://www.nxp.com/design/software/embedded-software/audio-video-bridging-software:AVB-SOFTWARE
- FreeRTOS builds require additional MCUXpresso SDK software: https://www.nxp.com/design/software/development-software/mcuxpresso-software-and-tools-/wired-communications-middleware-for-nxp-microcontrollers:WIRED-COMM-MIDDLEWARE?tab=Design_Tools_Tab

### FreeRTOS
Currently GenAVB/TSN stack support only ARM gcc toolchain for FreeRTOS targets.
To be able to build the stack, ARMGCC_DIR environement variable pointing
to arm-gcc toolchain must be defined.
```
export ARMGCC_DIR=/path/to/gcc-arm-none-eabi-xxx
```

The local config file should define:
```
set(FREERTOS_SDK "/path/to/freertos_MCUXpresso_SDK" CACHE PATH "Path to MCUXpresso SDK")
set(FREERTOS_APPS "/path/to/freertos_avb_apps" CACHE PATH "Path to FreeRTOS GenAVB/TSN apps repository")
```

### Linux
The Linux target is usually built using a complete toolchain, which helps
setting most of the environment variables required for cross-compilation.
In the case of a Yocto SDK/toolchain for example, the following command will
setup $CROSS_COMPILE as well as other related environment variables:

```
source /path/to/yocto/toolchain/environement-setup-xxx
```

The local config should however define:
```
set(KERNELDIR "/path/to/linux_avb" CACHE PATH "Path to Linux kernel")
set(NXP_SWITCH_PATH "/path/to/nxp_switch" CACHE PATH "Path to NXP switch driver"
```


Build commands
--------------
Syntax is:
```
cmake . -B<build directory> -DTARGET=<target name> -DCONFIG=<config name>
make -C <build directory> install
```

GenAVB/TSN stack provides also a shell function to facilitate the build process.
Usage is
```
# Setup environement, see Build paragraph above
source environement-genavb
make_genavb [target] [config]
```
where target and config are optional. If no config is defined, all available
configurations for specified target are built. If no target is specified the default
is linux_imx6.

### Generated binaries
The generated binaries are installed under \<build directory\>/

### Installing binaries to target

#### Linux
Copy the content of \<build directory\>/target/ to the root directory of the target filesystem

#### FreeRTOS
Refer to the FreeRTOS apps README.

