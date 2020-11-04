GenAVB/TSN
----------
GenAVB/TSN is a NXP generic AVB/TSN stack for NXP MCUs and MPUs. It is cross-platform,
currently supporting Linux and FreeRTOS.

Features
--------
- IEEE-802.1AS-2011 implementation, both time-aware Bridge and Endpoint support.
- IEEE-802.1Qat-2010 implementation.
- Binary protocol stacks running in standalone userspace processes.
- Public API provided by binary library plus header files.
- Example applications.

Repository structure
--------------------
- apps: example applications sources and makefiles.
- bin: protocol stack binaries and startup scripts.
- doc: API documentation (HTML Doxygen, follow index.html in doc/html directory)
- include: library header files
- lib: library binaries
- licenses: licenses files
- linux: kernel module sources and makefile


Building Example Applications
-----------------------------
General procedure:
1) install a cross-compilation toolchain
2) go to apps directory
3) run
```
make linux CROSS_COMPILE=<path_to_toolchain> STAGING_DIR=<path_to_staging_dir>
```
to build all the example applications present in the apps/linux directory, with
<path_to_toolchain> pointing to the cross-compilation toolchain and <path_to_staging_dir>
pointing to the current staging directory.

In the case of a Yocto SDK/toolchain, step 3 would then become:
```
source <PATH_TO_YOCTO_ENVIRONMENT_SETUP>
make linux STAGING_DIR=${SDKTARGETSYSROOT}
```

The binaries of the example applications are stored in the apps/build/linux repository.

When adding a new application, the name of the repository containing sources of
the new application should be added to apps/linux/Makefile.

Each example application can also be built from its own directory (run 'make'
in the application directory). In such a case the application binary will be
generated in the application directory.


