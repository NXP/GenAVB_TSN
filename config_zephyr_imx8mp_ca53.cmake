if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/local_config_${TARGET}.cmake)
  include(local_config_${TARGET}.cmake)
endif()

if(NOT CMAKE_TOOLCHAIN_FILE)
  message(FATAL_ERROR "No toolchain specified, make sure the correct one is selected")
else()
  message(STATUS "Toolchain: ${CMAKE_TOOLCHAIN_FILE}")
endif()

set(TARGET_OS freertos)
set(TARGET_SOC imx8)
set(TARGET_ARCH armv8a)

if(NOT DEFINED FREERTOS_SDK)
  message(WARNING "Undefined FREERTOS_SDK")
endif()

if(NOT DEFINED FREERTOS_DIR)
  message(WARNING "Undefined FREERTOS_DIR")
endif()

if(NOT DEFINED FREERTOS_APPS)
  message(WARNING "Undefined FREERTOS_APPS, default used")
endif()

if(NOT DEFINED RTOS_ABSTRACTION_LAYER_DIR)
  message(WARNING "Undefined RTOS_ABSTRACTION_LAYER_DIR")
endif()

set(FREERTOS_APP_INCLUDES  ${FREERTOS_APPS}/common ${FREERTOS_APPS}/common/freertos ${FREERTOS_APPS}/common/libs/hlog)
set(FREERTOS_BOARD_INCLUDE ${FREERTOS_APPS}/common/freertos/boards/evkmimx8mp ${AppBoardPath} ${AppPath}/common/boards/evkmimx8mp ${FREERTOS_APPS}/common/boards/evkmimx8mp)
set(FREERTOS_SDK_DEVICE_DIRS ${FREERTOS_SDK}/devices/MIMX8ML8 ${FREERTOS_SDK}/devices/MIMX8ML8/drivers)
set(FREERTOS_SDK_TARGET_DIRS ${FREERTOS_SDK}/CMSIS/Core_AArch64/Include ${FREERTOS_SDK}/drivers/cache/armv8-a)
set(FREERTOS_SDK_DRIVERS_DIRS ${FREERTOS_SDK}/drivers ${FREERTOS_SDK}/drivers/common ${FREERTOS_SDK}/drivers/enet_qos ${FREERTOS_SDK}/drivers/gpt)

set(INCLUDE_DIR ${FREERTOS_DIR}/include)

# Include the zephyr toolchain definitions early in source files
add_compile_options("-DCONFIG_ARM=1")
add_compile_options(-isystem "${ZEPHYR_BASE}/include")
# "SHELL" prefix to prevent de-duplication feature
add_compile_options(SHELL:-include "${ZEPHYR_BASE}/include/zephyr/toolchain/gcc.h")

add_compile_options(-O2 -Wall -g -Werror -Wpointer-arith -std=c99)
add_compile_options(-march=armv8-a)

