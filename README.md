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
| linux i.MX8     |      x       |    **O**     |    x   |    x   |
| linux LS1028    |              |              |  **O** |        |
| freertos RT1052 |      x       |      x       |        |        |
| freertos RT1176 |      x       |      x       |        |        |


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


Build requirements
------------------
- Linux host system and development tools (git, make, doxygen for the docs, ...)
- An ARM gcc toolchain is sufficient for the stack. However building the linux
applications requires a more complete cross-compilation SDK (because of
additional dependencies such as alsa, gstreamer and qt5).
- AVB endpoint builds depend on custom changes to the Linux kernel and Yocto distribution: https://www.nxp.com/design/software/embedded-software/audio-video-bridging-software:AVB-SOFTWARE
- FreeRTOS builds require additional MCUXpresso SDK software: https://www.nxp.com/design/software/development-software/mcuxpresso-software-and-tools-/wired-communications-middleware-for-nxp-microcontrollers:WIRED-COMM-MIDDLEWARE?tab=Design_Tools_Tab


Build configuration
-------------------
Some configuration is required to provide the path to the cross-compiler,
toolchain staging directory and other external components.

Local configuration files can be included by the build system in order to
define such variables specific to the developer environment, so that they don't
have to be specified at each invocation of make.
The local configuration file name for a given target is `./local_config_${target}.mk`.

### FreeRTOS
The following variables must be defined, either on the make command line or
(preferably) in the local configuration file:
```
FREERTOS_SDK=<path to FreeRTOS SDK>
FREERTOS_APPS=<path to FreeRTOS APPS>
export CROSS_COMPILE=<path to cross-compiler prefix>
```

### Linux
The Linux target is usually built using a complete toolchain, which helps
setting most of the environment variables required for cross-compilation.
In the case of a Yocto SDK/toolchain for example, the following command will
setup $CROSS_COMPILE as well as other related environment variables:

```
source <PATH_TO_YOCTO_ENVIRONMENT_SETUP>
```

The following variables must also be defined, either on the make command line or
(preferably) in the local configuration file:
```
KERNELDIR=<path to linux kernel source tree>
STAGING_DIR=<path to the toolchain staging dir> (${SDKTARGETSYSROOT} using Yocto SDK)
NXP_SWITCH_PATH=<path to NXP switch driver>
```
Note: the NXP_SWITCH_PATH variable is only needed when building for specific
hybrid or bridge configurations.

Build options
-------------
### target
The following targets are supported:
- linux_imx6
- linux_imx8
- linux_ls1028
- freertos_rt1052
- freertos_rt1176

### config
The build configurations can be found in ./configs.

The following configurations are supported:
- endpoint_avb (full featured AVB endpoint)
- endpoint_tsn 
- hybrid
- bridge

### Verbosity
By default the verbosity is minimal, so you can focus on what is actually built.

If more output is required use V parameter:
- V=0 is the default 'quiet' build.
- V=1 enables more debug output and full commands echoing.

Note: verbosity control doesn't affect applications (linux).

Build commands
--------------
Syntax is:
```
make target=<target name> config=<config name> <build target>
```

### Supported build targets
- all
- install
- clean
- stack
- stack-install
- stack-clean
- apps

If target is not specified, the default is linux_imx6.
If config is not specified, the default is to build all the configurations
available for the given target.

### Generated binaries
Stack: The generated binaries are installed under build/\<target name\>/\<config name\>/.

Example applications: The  generated binaries are placed in
build/\<target name\>/\<config name\>/apps/linux/\<app name\>/.

### Installing binaries to target

#### Linux
Stack: Copy the content of build/\<target name\>/\<config name\>/target/ to the
root directory of the target filesystem.

Example applications: the generated binaries must be copied to the target
filesystem.

#### FreeRTOS
Refer to the FreeRTOS apps README.

