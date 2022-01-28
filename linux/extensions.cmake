
function(genavb_add_executable)
  cmake_parse_arguments(ARG "" "NAME" "SRCS" ${ARGN})

  if(NOT DEFINED ARG_NAME)
    return()
  endif()

  foreach(src IN LISTS ARG_SRCS)
    list(APPEND srcs "${CMAKE_CURRENT_LIST_DIR}/${src}")
  endforeach()

  add_executable(${ARG_NAME} ${srcs})

  target_compile_definitions(${ARG_NAME} PRIVATE _COMPONENT_ID_=os_COMPONENT_ID)
  target_compile_definitions(${ARG_NAME} PRIVATE _COMPONENT_STR_=\"${ARG_NAME}\")
  target_compile_definitions(${ARG_NAME} PRIVATE _COMPONENT_=${ARG_NAME}_)

  target_compile_options(${ARG_NAME} PRIVATE -no-pie)
  target_link_libraries(${ARG_NAME} m)
  target_link_libraries(${ARG_NAME} pthread)

  install(TARGETS ${ARG_NAME} DESTINATION ${BIN_DIR})
endfunction()

function(genavb_add_shared_library)
  cmake_parse_arguments(ARG "" "NAME" "SRCS" ${ARGN})

  if(NOT DEFINED ARG_NAME)
    return()
  endif()

  foreach(src IN LISTS ARG_SRCS)
    list(APPEND srcs "${CMAKE_CURRENT_LIST_DIR}/${src}")
  endforeach()

  add_library(${ARG_NAME} SHARED ${srcs})

  target_link_libraries(${ARG_NAME} -Wl,--no-undefined)

  target_compile_definitions(${ARG_NAME} PRIVATE _COMPONENT_ID_=os_COMPONENT_ID)
  target_compile_definitions(${ARG_NAME} PRIVATE _COMPONENT_STR_=\"${ARG_NAME}\")
  target_compile_definitions(${ARG_NAME} PRIVATE _COMPONENT_=${ARG_NAME}_)

  set_target_properties(${ARG_NAME} PROPERTIES VERSION 1.0)
  set_target_properties(${ARG_NAME} PROPERTIES SOVERSION 1)
  install(TARGETS ${ARG_NAME} DESTINATION ${LIB_DIR})
endfunction()

function(genavb_add_dependencies)
  cmake_parse_arguments(ARG "" "TARGET" "DEP" ${ARGN})
  if(NOT DEFINED ARG_TARGET OR NOT TARGET ${ARG_TARGET})
    return()
  endif()
  add_dependencies(${ARG_TARGET} ${ARG_DEP})
endfunction()

function(genavb_generate_archives)
  add_custom_target(avb-core
    DEPENDS ${genavb_used_libs}
    )
endfunction()

