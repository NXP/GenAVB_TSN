if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/local_config_${TARGET}.cmake)
  include(local_config_${TARGET}.cmake)
endif()

if(NOT CMAKE_TOOLCHAIN_FILE)
  message(FATAL_ERROR "No toolchain specified, make sure the correct one is selected")
else()
  message(STATUS "Toolchain: ${CMAKE_TOOLCHAIN_FILE}")
endif()

set(TARGET_OS freertos)
set(TARGET_SOC rt1189)
set(TARGET_ARCH armv8m)

if(NOT DEFINED FREERTOS_SDK)
  message(WARNING "Undefined FREERTOS_SDK")
endif()

if(NOT DEFINED FREERTOS_APPS)
  message(WARNING "Undefined FREERTOS_APPS, default used")
endif()

if(EXISTS ${FREERTOS_SDK}/rtos/freertos/freertos_kernel)
set(FREERTOS_DIR ${FREERTOS_SDK}/rtos/freertos/freertos_kernel)
else()
set(FREERTOS_DIR ${FREERTOS_SDK}/rtos/freertos/freertos-kernel)
endif()
set(FREERTOS_PORT GCC/ARM_CM33_NTZ/non_secure)
set(FREERTOS_APP_INCLUDES ${FREERTOS_APPS}/boards/src/demo_apps/avb_tsn/common/cm33 ${FREERTOS_APPS}/boards/src/demo_apps/avb_tsn/common)
set(FREERTOS_BOARD_INCLUDE ${FREERTOS_APPS}/boards/evkmimxrt1180/demo_apps/avb_tsn/common/cm33)
set(FREERTOS_DEVICE ${FREERTOS_SDK}/devices/MIMXRT1189)
set(FREERTOS_SDK_DEVICE_DIRS ${FREERTOS_SDK}/devices/MIMXRT1189 ${FREERTOS_SDK}/devices/MIMXRT1189/drivers ${FREERTOS_SDK}/devices/MIMXRT1189/drivers/cm33)
set(FREERTOS_SDK_TARGET_DIRS ${FREERTOS_SDK}/CMSIS/Core/Include)

set(INCLUDE_DIR usr/include)

add_compile_definitions(CPU_MIMXRT1189CVM8A_cm33)
add_compile_options(-Wall -g -Werror -Wpointer-arith -std=c99 -Wdeclaration-after-statement)
add_compile_options(-Os -mthumb -mcpu=cortex-m33 -mfloat-abi=hard -mfpu=fpv5-sp-d16)
