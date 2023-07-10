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

set(FREERTOS_PORT GCC/ARM_CA53_64_BIT)
set(FREERTOS_APP_INCLUDES  ${FREERTOS_APPS}/common ${FREERTOS_APPS}/common/freertos ${FREERTOS_APPS}/common/libs/hlog)
set(FREERTOS_BOARD_INCLUDE ${FREERTOS_APPS}/common/freertos/boards/evkmimx8mn ${AppBoardPath} ${AppPath}/common/boards/evkmimx8mn)
set(FREERTOS_SDK_DEVICE_DIRS ${FREERTOS_SDK}/devices/MIMX8MN6 ${FREERTOS_SDK}/devices/MIMX8MN6/drivers)
set(FREERTOS_SDK_TARGET_DIRS ${FREERTOS_SDK}/CMSIS/Core_AArch64/Include ${FREERTOS_SDK}/drivers/cache/armv8-a)
set(FREERTOS_SDK_DRIVERS_DIRS ${FREERTOS_SDK}/drivers ${FREERTOS_SDK}/drivers/common ${FREERTOS_SDK}/drivers/enet ${FREERTOS_SDK}/drivers/gpt ${FREERTOS_SDK}/components/phy/mdio/enet ${FREERTOS_SDK}/components/phy/device/phyar8031)

set(INCLUDE_DIR ${FREERTOS_DIR}/include)

add_compile_definitions(GUEST CPU_MIMX8MN6DVTJZ_ca53 ENET_ENHANCEDBUFFERDESCRIPTOR_MODE)
add_compile_options(-O0 -Wall -g -Werror -Wpointer-arith -std=c99 -Wdeclaration-after-statement)
add_compile_options(-march=armv8-a)

