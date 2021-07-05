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


Build configuration
-------------------
Some preliminary configuration is required to provide path to the toolchain, 
staging directory and external components (see External dependencies below).

Local configuration files can be included by the build system in order to
to define some variables specific to the developer environment.
The local config file name is `./local_config_${target}.mk`

### FreeRTOS
The local config file should define:
```
FREERTOS_SDK=<path to FreeRTOS SDK>
FREERTOS_APPS=<path to FreeRTOS APPS>
export CROSS_COMPILE=<path to cross-compiler prefix>
```

### Linux
Usually Linux target is built using a complete Yocto SDK toolchain which sets
environment variables for cross-compilation.

The local config should however define:
```
KERNELDIR=<path to linux AVB>
STAGING_DIR=<path to staging dir> (${SDKTARGETSYSROOT} using Yocto SDK)
NXP_SWITCH_PATH=<path to NXP switch>
```


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

If target is not specified, the default is linux_imx6.
If config is not specified, the default is to build all the configurations for
the given target.

### Generated binaries

The generated binaries are installed under build/\<target name\>/\<config name\>/

### Installing binaries to target

#### Linux
Copy the content of build/\<target name\>/\<config name\>/target/ to the root directory of the target filesystem

#### FreeRTOS
Refer to the FreeRTOS apps README

