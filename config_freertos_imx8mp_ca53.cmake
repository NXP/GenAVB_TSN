if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/local_config_${TARGET}.cmake)
  include(local_config_${TARGET}.cmake)
endif()

if(NOT CMAKE_TOOLCHAIN_FILE)
  message(FATAL_ERROR "No toolchain specified, make sure the correct one is selected")
else()
  message(STATUS "Toolchain: ${CMAKE_TOOLCHAIN_FILE}")
endif()

set(TARGET_OS rtos)
set(TARGET_SOC imx8)
set(TARGET_ARCH armv8a)

set(RTOS_ABSTRACTION_LAYER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/rtos/freertos)

if(NOT DEFINED MCUX_SDK)
  message(WARNING "Undefined MCUX_SDK")
endif()

if(NOT DEFINED RTOS_DIR)
  message(WARNING "Undefined RTOS_DIR")
endif()

if(NOT DEFINED RTOS_APPS)
  message(WARNING "Undefined RTOS_APPS, default used")
endif()

set(FREERTOS_PORT GCC/ARM_CA53_64_BIT)
set(FREERTOS_CONFIG_INCLUDES ${RTOS_APPS}/common/freertos)
set(APP_GENAVB_SDK_INCLUDE ${AppPath}/common/boards/evkmimx8mp)
set(MCUX_SDK_DEVICE_DIRS ${MCUX_SDK}/devices/MIMX8ML8 ${MCUX_SDK}/devices/MIMX8ML8/drivers)
set(MCUX_SDK_TARGET_DIRS ${MCUX_SDK}/CMSIS/Core_AArch64/Include ${MCUX_SDK}/drivers/cache/armv8-a)
set(MCUX_SDK_DRIVERS_DIRS ${MCUX_SDK}/drivers ${MCUX_SDK}/drivers/common ${MCUX_SDK}/drivers/enet ${MCUX_SDK}/drivers/enet_qos ${MCUX_SDK}/drivers/gpt)

set(INCLUDE_DIR ${RTOS_DIR}/include)

add_compile_definitions(GUEST CPU_MIMX8ML8DVNLZ_ca53 ENET_ENHANCEDBUFFERDESCRIPTOR_MODE)
add_compile_options(-O0 -Wall -g -Werror -Wpointer-arith -std=c99 -Wdeclaration-after-statement)
add_compile_options(-march=armv8-a)

