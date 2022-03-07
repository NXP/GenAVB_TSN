
function(genavb_add_executable)
  cmake_parse_arguments(ARG "" "NAME" "SRCS" ${ARGN})

  if(NOT DEFINED ARG_NAME)
    return()
  endif()

  foreach(src IN LISTS ARG_SRCS)
    list(APPEND srcs "${CMAKE_CURRENT_LIST_DIR}/${src}")
  endforeach()

  add_executable(${ARG_NAME} ${srcs})

  add_dependencies(stack ${ARG_NAME})

  target_compile_definitions(${ARG_NAME} PRIVATE _COMPONENT_ID_=os_COMPONENT_ID)
  target_compile_definitions(${ARG_NAME} PRIVATE _COMPONENT_STR_=\"${ARG_NAME}\")
  target_compile_definitions(${ARG_NAME} PRIVATE _COMPONENT_=${ARG_NAME}_)

  target_link_libraries(${ARG_NAME} -no-pie)
  target_link_libraries(${ARG_NAME} m)
  target_link_libraries(${ARG_NAME} pthread)

  set_target_properties(${ARG_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

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

  add_dependencies(stack ${ARG_NAME})

  target_link_libraries(${ARG_NAME} -Wl,--no-undefined)

  target_compile_definitions(${ARG_NAME} PRIVATE _COMPONENT_ID_=os_COMPONENT_ID)
  target_compile_definitions(${ARG_NAME} PRIVATE _COMPONENT_STR_=\"${ARG_NAME}\")
  target_compile_definitions(${ARG_NAME} PRIVATE _COMPONENT_=${ARG_NAME}_)

  set_target_properties(${ARG_NAME} PROPERTIES VERSION 1.0)
  set_target_properties(${ARG_NAME} PROPERTIES SOVERSION 1)

  set_target_properties(${ARG_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)

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
  add_custom_target(stack-core
    DEPENDS ${genavb_used_libs}
    )
endfunction()

function(genavb_install)
  set(common_headers "${CMAKE_CURRENT_SOURCE_DIR}/include/genavb/*.h")
  set(os_headers "${CMAKE_CURRENT_SOURCE_DIR}/include/${TARGET_OS}/os/*.h")

  # prepare include dir for applications
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/include
    COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/include/genavb/os
    COMMAND cp -a ${common_headers} ${CMAKE_CURRENT_BINARY_DIR}/include/genavb
    COMMAND cp -a ${os_headers} ${CMAKE_CURRENT_BINARY_DIR}/include/genavb/os
    COMMAND touch ${CMAKE_CURRENT_BINARY_DIR}/include
    DEPENDS ${common_headers} ${os_headers}
    COMMENT "Generate includes"
  )

  add_custom_target(include-prep ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/include
  )

  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/ DESTINATION ${INCLUDE_DIR})
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin/ DESTINATION ${BIN_DIR} FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib/ DESTINATION ${LIB_DIR} FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
endfunction()
