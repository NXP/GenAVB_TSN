if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/local_config_${TARGET}.cmake)
  include(local_config_${TARGET}.cmake)
endif()

if(NOT CMAKE_TOOLCHAIN_FILE)
  message(FATAL_ERROR "No toolchain specified, make sure the correct one is selected")
else()
  message(STATUS "Toolchain: ${CMAKE_TOOLCHAIN_FILE}")
endif()

set(TARGET_OS freertos)
set(TARGET_SOC rt1052)
set(TARGET_ARCH armv7m)

if(NOT DEFINED FREERTOS_SDK)
  message(WARNING "Undefined FREERTOS_SDK")
endif()

if(NOT DEFINED FREERTOS_APPS)
  message(WARNING "Undefined FREERTOS_APPS, default used")
endif()

set(FREERTOS_DIR ${FREERTOS_SDK}/rtos/freertos/freertos_kernel)
set(FREERTOS_PORT GCC/ARM_CM4F)
set(FREERTOS_APP_INCLUDES ${FREERTOS_APPS}/boards/src/demo_apps/avb_tsn/common)
set(FREERTOS_BOARD_INCLUDE ${FREERTOS_APPS}/boards/evkbimxrt1050/demo_apps/avb_tsn/common)
set(FREERTOS_SDK_DEVICE_DIRS ${FREERTOS_SDK}/devices/MIMXRT1052 ${FREERTOS_SDK}/devices/MIMXRT1052/drivers ${FREERTOS_SDK}/devices/MIMXRT1052/drivers/cm7)
set(FREERTOS_SDK_TARGET_DIRS ${FREERTOS_SDK}/CMSIS/Core/Include)

set(INCLUDE_DIR usr/include)

add_compile_definitions(CPU_MIMXRT1052DVL6B ENET_ENHANCEDBUFFERDESCRIPTOR_MODE CFG_PHY_OLD_INTERFACE)
add_compile_options(-Wall -g -Werror -Wpointer-arith -std=c99 -Wdeclaration-after-statement)
add_compile_options(-Os -mthumb -mcpu=cortex-m7 -mfloat-abi=hard -mfpu=fpv5-d16)

