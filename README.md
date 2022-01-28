GenAVB/TSN
----------
GenAVB/TSN is a NXP generic AVB/TSN stack for NXP MCUs and MPUs. It is cross-platform,
currently supporting Linux and FreeRTOS.


Development environment
-----------------------
- Linux host system and development tools (git, make, doxygen for the docs, ...)
- ARM gcc toolchain is sufficient for the stack. However building the linux
applications require a more complete cross-compilation SDK (alsa, gstreamer and qt5 dependencies)


Repository structure
--------------------
- api:      the public API
- apps:     demo applications
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
Usually Linux target is built using a complete Yocto SDK toolchain which sets
environment variables for cross-compilation.

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

### target
The following targets are supported:
- linux_imx6
- linux_imx8
- linux_ls1028
- freertos_rt1052
- freertos_rt1176

### config
The build configurations can be found in ./configs directory.

The following configurations are supported:
- endpoint_avb (full featured AVB endpoint)
- endpoint_tsn
- hybrid
- bridge


### Generated binaries

The generated binaries are installed under \<build directory\>/

### Installing binaries to target

#### Linux
Copy the content of \<build directory\>/target/ to the root directory of the target filesystem

#### FreeRTOS
Refer to the FreeRTOS apps README

