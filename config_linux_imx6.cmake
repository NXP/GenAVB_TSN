if(EXISTS local_config_${TARGET}.cmake)
  include(local_config_${TARGET}.cmake)
endif()

if(NOT CMAKE_TOOLCHAIN_FILE)
  message(FATAL_ERROR "No toolchain specified, make sure the correct one is selected")
else()
  message(STATUS "Toolchain: ${CMAKE_TOOLCHAIN_FILE}")
endif()

set(TARGET_OS linux)
set(TARGET_SOC imx6)
set(TARGET_ARCH armv7a)

if(NOT DEFINED KERNELDIR)
  message(WARNING "No KERNELDIR specified")
endif()

if(NOT DEFINED NXP_SWITCH_PATH)
  message(WARNING "No NXP_SWITCH_PATH specified")
endif()

set(BIN_DIR usr/bin)
set(LIB_DIR usr/lib)
set(INCLUDE_DIR usr/include)
set(CONFIG_DIR etc/genavb)
set(INITSCRIPT_DIR etc/init.d)
set(FIRMWARE_DIR lib/firmware/genavb)

add_compile_options(-O2 -Wall -g -Werror -Wpointer-arith -std=c99 -Wdeclaration-after-statement)
