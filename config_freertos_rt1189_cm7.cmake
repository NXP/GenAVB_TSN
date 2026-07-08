if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/local_config_${TARGET}.cmake)
  include(local_config_${TARGET}.cmake)
endif()

if(NOT CMAKE_TOOLCHAIN_FILE)
  message(FATAL_ERROR "No toolchain specified, make sure the correct one is selected")
else()
  message(STATUS "Toolchain: ${CMAKE_TOOLCHAIN_FILE}")
endif()

set(TARGET_OS rtos)
set(TARGET_ARCH armv8m)

if(NOT DEFINED MCUX_SDK)
  message(FATAL_ERROR "Undefined MCUX_SDK")
endif()

if(NOT DEFINED RTOS_APPS)
  message(FATAL_ERROR "Undefined RTOS_APPS")
endif()

if(NOT DEFINED RTOS_ABSTRACTION_LAYER_DIR)
  message(FATAL_ERROR "Undefined RTOS_ABSTRACTION_LAYER_DIR")
endif()

set(RTOS_DIR ${MCUX_SDK}/rtos/freertos/freertos-kernel)
set(FREERTOS_PORT GCC/ARM_CM4F)
if(NOT DEFINED FREERTOS_CONFIG_INCLUDES)
set(FREERTOS_CONFIG_INCLUDES ${RTOS_APPS}/boards/evkmimxrt1180/demo_apps/avb_tsn/common/cm7 ${RTOS_APPS}/devices/MIMXRT118x/common/cm7 ${RTOS_APPS}/boards/src/demo_apps)
endif()
if(NOT DEFINED APP_GENAVB_SDK_INCLUDE)
set(APP_GENAVB_SDK_INCLUDE ${RTOS_APPS}/boards/evkmimxrt1180/demo_apps/avb_tsn/tsn_app/cm7 ${RTOS_APPS}/devices/MIMXRT118x/common/cm7)
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
  ${MCUX_SDK}/drivers/tpm
  ${MCUX_SDK}/devices/RT/RT1180/periph
  ${MCUX_SDK}/devices/RT/RT1180/MIMXRT1189/drivers
  ${MCUX_SDK}/devices/RT/RT1180/MIMXRT1189
)
set(MCUX_SDK_TARGET_DIRS ${MCUX_SDK}/arch/arm/CMSIS/Core/Include)

set(INCLUDE_DIR usr/include)

add_compile_definitions(CPU_MIMXRT1189CVM8B_cm7)
add_compile_options(-Wall -g -Werror -Wpointer-arith -std=c99 -Wdeclaration-after-statement)
add_compile_options(-Os -mthumb -mcpu=cortex-m7 -mfloat-abi=hard -mfpu=fpv5-sp-d16)
