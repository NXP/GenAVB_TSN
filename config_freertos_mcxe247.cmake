if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/local_config_${TARGET}.cmake)
  include(local_config_${TARGET}.cmake)
endif()

if(NOT CMAKE_TOOLCHAIN_FILE)
  message(FATAL_ERROR "No toolchain specified, make sure the correct one is selected")
else()
  message(STATUS "Toolchain: ${CMAKE_TOOLCHAIN_FILE}")
endif()

set(TARGET_OS rtos)
set(TARGET_ARCH armv7m)

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
set(FREERTOS_CONFIG_INCLUDES ${RTOS_APPS}/boards/frdmmcxe247/demo_apps/avb_tsn/common/ ${RTOS_APPS}/boards/src/demo_apps)
endif()
if(NOT DEFINED APP_GENAVB_SDK_INCLUDE)
set(APP_GENAVB_SDK_INCLUDE ${RTOS_APPS}/boards/frdmmcxe247/demo_apps/avb_tsn/common/)
endif()

set(MCUX_SDK_DEVICE_DIRS
  ${MCUX_SDK}/drivers/cache/lmem
  ${MCUX_SDK}/drivers/enet
  ${MCUX_SDK}/drivers/ftm
  ${MCUX_SDK}/drivers/common
  ${MCUX_SDK}/components/phy
  ${MCUX_SDK}/devices/MCX/MCXE/periph0
  ${MCUX_SDK}/devices/MCX/MCXE/MCXE247/drivers
  ${MCUX_SDK}/devices/MCX/MCXE/MCXE247
)
set(MCUX_SDK_TARGET_DIRS ${MCUX_SDK}/arch/arm/CMSIS/Core/Include)

set(INCLUDE_DIR usr/include)

add_compile_definitions(CPU_MCXE247VLQ ENET_ENHANCEDBUFFERDESCRIPTOR_MODE)
add_compile_options(-Wall -g -Werror -Wpointer-arith -std=c99)
add_compile_options(-Os -mthumb -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16)
