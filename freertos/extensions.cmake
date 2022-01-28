
# genavb_add_os_library(NAME <target> SRCS <src1 src2 ...>)
function(genavb_add_os_library)
  cmake_parse_arguments(ARG "" "NAME" "SRCS" ${ARGN})

  if(NOT DEFINED ARG_NAME)
    return()
  endif()

  foreach(src IN LISTS ARG_SRCS)
    list(APPEND srcs "${CMAKE_CURRENT_LIST_DIR}/${src}")
  endforeach()

  add_library(${ARG_NAME} STATIC ${srcs})

  target_compile_definitions(${ARG_NAME} PRIVATE _COMPONENT_ID_=os_COMPONENT_ID)
  target_compile_definitions(${ARG_NAME} PRIVATE _COMPONENT_STR_=\"${ARG_NAME}\")
  target_compile_definitions(${ARG_NAME} PRIVATE _COMPONENT_=${ARG_NAME}_)

endfunction()

function(genavb_generate_archives)
  set(lib_output "${CMAKE_CURRENT_BINARY_DIR}/libavb-core.a")
  set(script_file "${CMAKE_CURRENT_BINARY_DIR}/ar-script.mri")

  # Generate ar script to merge core libs into one libavb-core.a
  set(script "create ${lib_output}\n")
  foreach(lib IN LISTS genavb_used_libs)
    # In release mode ${lib} is a precompiled library
    if(TARGET ${lib})
      set(lib "$<TARGET_FILE:${lib}>")
    endif()

    set(script "${script}addlib ${lib}\n")
  endforeach()
  set(script "${script}save\nend\n")

  file(GENERATE OUTPUT ${script_file} CONTENT ${script})

  add_custom_command(
    OUTPUT ${lib_output}
    COMMAND ${CMAKE_AR} -M < ${script_file}
    DEPENDS ${genavb_used_libs}
    VERBATIM
    )

  add_custom_target(avb-core ALL
    DEPENDS ${lib_output}
    )
endfunction()

