if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/local_config_${TARGET}.cmake)
  include(local_config_${TARGET}.cmake)
endif()

set(TARGET_OS rtos)
set(TARGET_ARCH armv8m)

if(NOT DEFINED MCUX_SDK)
  message(FATAL_ERROR "Undefined MCUX_SDK")
endif()

if(NOT DEFINED RTOS_DIR)
  message(WARNING "Undefined RTOS_DIR")
endif()

if(NOT DEFINED RTOS_APPS)
  message(FATAL_ERROR "Undefined RTOS_APPS")
endif()

if(NOT DEFINED RTOS_ABSTRACTION_LAYER_DIR)
  message(FATAL_ERROR "Undefined RTOS_ABSTRACTION_LAYER_DIR")
endif()

if(NOT DEFINED APP_GENAVB_SDK_INCLUDE)
  set(APP_GENAVB_SDK_INCLUDE ${RTOS_APPS}/boards/evkmimxrt1180/cm33)
endif()

set(MCUX_SDK_DEVICE_DIRS
  ${MCUX_SDK}/drivers/cache/xcache
  ${MCUX_SDK}/drivers/gpt
  ${MCUX_SDK}/drivers/msgintr
  ${MCUX_SDK}/drivers/netc
  ${MCUX_SDK}/drivers/netc/socs/imxrt1180
  ${MCUX_SDK}/drivers/netc/netc_hw
  ${MCUX_SDK}/drivers/common
  ${MCUX_SDK}/components/phy
  ${MCUX_SDK}/devices/RT/RT1180/periph
  ${MCUX_SDK}/devices/RT/RT1180/MIMXRT1189/drivers
  ${MCUX_SDK}/devices/RT/RT1180/MIMXRT1189
)
set(MCUX_SDK_TARGET_DIRS ${MCUX_SDK}/arch/arm/CMSIS/Core/Include)

set(INCLUDE_DIR ${RTOS_DIR}/include)

# Include the zephyr toolchain definitions early in source files
add_compile_options("-DCONFIG_ARM=1")
add_compile_options(-isystem "${ZEPHYR_BASE}/include")
# "SHELL" prefix to prevent de-duplication feature
add_compile_options(SHELL:-include "${ZEPHYR_BASE}/include/zephyr/toolchain.h")

add_compile_definitions(CPU_MIMXRT1189CVM8B_cm33)
add_compile_options(-Wall -g -Werror -Wpointer-arith -std=c99)
add_compile_options(-Os -mthumb -mcpu=cortex-m33 -mfloat-abi=hard -mfpu=fpv5-sp-d16)
