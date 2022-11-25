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

| Compilation target     | Hardware target                      | AVB endpoint | TSN endpoint | AVB/TSN bridge |
| :--------------------- | :----------------------------------- | :----------: | :----------: | :------------: |
| linux i.MX6            | i.MX 6Q, i.MX 6QP, i.MX6D, i.MX6SX   |      x       |              |                |
| linux i.MX6ULL         | i.MX 6ULL                            |    **O**     |              |                |
| linux i.MX8            | i.MX 8MM, i.MX 8MP, i.MX 93          |    **O**     |    **O**     |                |
| linux LS1028           | LS1028A                              |              |              |      **O**     |
| freertos RT1052        | i.MX RT1052                          |      x       |      x       |                |
| freertos RT1176        | i.MX RT1176                          |      x       |      x       |                |
| freertos RT1189 M33    | i.MX RT1189                          |              |              |        x       |
| freertos i.MX8MM A53   | i.MX 8MM                             |    **O**     |    **O**     |                |
| freertos i.MX8MN A53   | i.MX 8MN                             |    **O**     |    **O**     |                |
| freertos i.MX8MP A53   | i.MX 8MP                             |    **O**     |    **O**     |                |

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
- avdecc:   IEEE 1722.1-2013/Milan 1.1a component stack
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
additional dependencies such as alsa, gstreamer and qt5/qt6).
- AVB endpoint builds depend on custom changes to the Linux kernel and specific Yocto distribution: https://www.nxp.com/design/software/development-software/real-time-edge-software:REALTIME-EDGE-SOFTWARE
- FreeRTOS builds require additional MCUXpresso SDK software: https://www.nxp.com/design/software/development-software/mcuxpresso-software-and-tools-/wired-communications-middleware-for-nxp-microcontrollers:WIRED-COMM-MIDDLEWARE?fpsp=1&#avb-tsn

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
set(FREERTOS_APPS "/path/to/freertos_application" CACHE PATH "Path to FreeRTOS application repository")
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
```


Build commands
--------------
Syntax is:
```
cmake . -B<build directory> -DTARGET=<target name> -DCONFIG=<config name>
make -C <build directory> install
```

The GenAVB/TSN stack also provides a couple shell functions with auto-completion
to facilitate the build process.
Usage is
```
# Setup environment, see Build paragraph above
source environment-genavb
make_genavb [target] [config_list]
clean_genavb [target]
```
where target and config_list are optional. If no config_list is defined, all available
configurations for specified target are built. If no target is specified the default
is linux_imx6.
config_list is of the form: configA configB ..., with one or more members.


### Generated binaries
The generated binaries are installed under \<build directory\>/

### Installing binaries to target

#### Linux
Copy the content of \<build directory\>/target/ to the root directory of the target filesystem

#### FreeRTOS
Refer to the FreeRTOS application README.

