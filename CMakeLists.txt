cmake_minimum_required(VERSION 3.10)

project(GENAVB/TSN)

set(TOPDIR ${CMAKE_CURRENT_SOURCE_DIR})

set(VERSION_FILE common/version.h)

if(NOT DEFINED TARGET)
  message(FATAL_ERROR "Target not defined")
endif()
message(STATUS "TARGET: ${TARGET}")

set(target_file "${CMAKE_CURRENT_SOURCE_DIR}/config_${TARGET}.cmake")
if(NOT EXISTS ${target_file})
  message(FATAL_ERROR "cannot find ${target_file}")
endif()

include(configs/configs.cmake)
include(${target_file})
include(extensions.cmake)

list(APPEND maindirs
  ${TARGET_OS}
  management
  gptp
  srp
  maap
  avtp
  avdecc
  api
  public
  common
)

foreach(dir ${maindirs})
  include(${dir}/${dir}.cmake)
endforeach()

include(CMakeLists.${TARGET_OS})


genavb_generate_archives()

# Install includes
install(DIRECTORY DESTINATION ${INCLUDE_DIR}/genavb/os)
install(DIRECTORY include/genavb DESTINATION ${INCLUDE_DIR})
install(DIRECTORY include/${TARGET_OS}/os DESTINATION ${INCLUDE_DIR}/genavb)
