function(genavb_set_option option value)
  option(${option} "" ${value})

  if(${option})
    set(cmake_config_defines ${cmake_config_defines} -D${option}=1)
    set(cmake_config_options ${cmake_config_options} ${option}=y)

    add_definitions(-D${option}=1)
  endif()
endfunction()

if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/${CONFIG}.cmake)
  include(${CMAKE_CURRENT_LIST_DIR}/${CONFIG}.cmake)
else()
  message(FATAL_ERROR "Configuration file for ${CONFIG} does not exist")
endif()

message(STATUS "CONFIG: ${CONFIG}")
