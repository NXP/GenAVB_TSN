include(${TARGET_OS}/extensions.cmake)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -iquote ${TOPDIR}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -iquote ${TOPDIR}/${TARGET_OS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -iquote ${TOPDIR}/common")

include_directories("${TOPDIR}/include")
include_directories("${TOPDIR}/include/${TARGET_OS}")

if(CONFIG_API)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -iquote ${TOPDIR}/api/${TARGET_OS}")
endif()

# genavb_add_library(NAME <target> SRCS <src1 src2 ...>)
function(genavb_add_library)
  cmake_parse_arguments(ARG "" "NAME" "SRCS" ${ARGN})
  if(NOT DEFINED ARG_NAME)
    return()
  endif()

  foreach(src IN LISTS ARG_SRCS)
    list(APPEND srcs "${CMAKE_CURRENT_LIST_DIR}/${src}")
  endforeach()

  add_library(${ARG_NAME} STATIC ${srcs})

  if(ARCHIVE_OUTPUT_DIR)
    set_target_properties(${ARG_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${ARCHIVE_OUTPUT_DIR}/${ARG_NAME})
    set_target_properties(${ARG_NAME} PROPERTIES OUTPUT_NAME "${ARG_NAME}-${TARGET_ARCH}")
  endif()

  target_compile_definitions(${ARG_NAME} PRIVATE _COMPONENT_ID_=${ARG_NAME}_COMPONENT_ID)
  target_compile_definitions(${ARG_NAME} PRIVATE _COMPONENT_STR_=\"${ARG_NAME}\")
  target_compile_definitions(${ARG_NAME} PRIVATE _COMPONENT_=${ARG_NAME}_)

  target_compile_options(${ARG_NAME} PRIVATE -include ${CMAKE_CURRENT_LIST_DIR}/config.h)

  add_custom_command(TARGET ${ARG_NAME} POST_BUILD COMMAND
    ${CMAKE_OBJCOPY} $<TARGET_FILE:${ARG_NAME}>
    --rename-section .text=.text.${ARG_NAME}
    --rename-section .data=.data.${ARG_NAME}
    --rename-section .rodata=.rodata.${ARG_NAME}
    --rename-section .bss=.bss.${ARG_NAME}
    )
endfunction()

# genavb_link_libraries(TARGET <target> LIB <library>)
function(genavb_link_libraries)
  cmake_parse_arguments(ARG "" "TARGET" "LIB" ${ARGN})
  if(NOT DEFINED ARG_TARGET OR NOT TARGET ${ARG_TARGET})
    return()
  endif()

  # In release mode, LIB is precompiled library
  if(NOT TARGET ${ARG_LIB})
    set(ARG_LIB ${CMAKE_CURRENT_LIST_DIR}/${ARG_LIB})
  endif()

  target_link_libraries(${ARG_TARGET} ${ARG_LIB})

  # Store all libraries used in current build
  if(NOT ${ARG_LIB} IN_LIST genavb_used_libs)
    set(genavb_used_libs ${genavb_used_libs} ${ARG_LIB} PARENT_SCOPE)
  endif()
endfunction()

# genavb_target_add_srcs(TARGET <target> SRCS <src1 src2 ...>)
function(genavb_target_add_srcs)
  cmake_parse_arguments(ARG "" "TARGET" "SRCS" ${ARGN})
  if(NOT DEFINED ARG_TARGET OR NOT TARGET ${ARG_TARGET})
    return()
  endif()

  foreach(src IN LISTS ARG_SRCS)
    target_sources(${ARG_TARGET} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/${src})
  endforeach()
endfunction()

function(genavb_add_os_component_defines target)
  target_compile_definitions(${target} PRIVATE _COMPONENT_ID_=os_COMPONENT_ID)
  target_compile_definitions(${target} PRIVATE _COMPONENT_STR_=\"os\")
  target_compile_definitions(${target} PRIVATE _COMPONENT_=os_)
endfunction()

function(genavb_conditional_include file)
  if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/${file})
    include(${CMAKE_CURRENT_LIST_DIR}/${file})
  endif()
endfunction()

function(genavb_include_os file)
  include(${CMAKE_CURRENT_LIST_DIR}/${file})
endfunction()

function(genavb_generate_version)
  if(NOT DEFINED genavb_git_version)
    set(genavb_git_version "unknown")
  endif()

  if(EXISTS ${TOPDIR}/.git)
    execute_process(COMMAND git describe --tags --exact-match ERROR_QUIET WORKING_DIRECTORY ${TOPDIR} RESULT_VARIABLE exact_match_result OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT exact_match_result)
      execute_process(COMMAND git describe --always --tags --dirty WORKING_DIRECTORY ${TOPDIR} RESULT_VARIABLE decribe_tag_result OUTPUT_VARIABLE genavb_git_version OUTPUT_STRIP_TRAILING_WHITESPACE)
    else()
      set(postfix "")
      execute_process(COMMAND git diff-index --quiet HEAD WORKING_DIRECTORY ${TOPDIR} RESULT_VARIABLE postfix_result)
      if(NOT postfix_result)
        set(postfix "-dirty")
      endif()
      execute_process(COMMAND git rev-parse --abbrev-ref HEAD WORKING_DIRECTORY ${TOPDIR} RESULT_VARIABLE branch_result OUTPUT_VARIABLE branch OUTPUT_STRIP_TRAILING_WHITESPACE)
      execute_process(COMMAND git rev-parse --verify --short HEAD WORKING_DIRECTORY ${TOPDIR} RESULT_VARIABLE commit_result OUTPUT_VARIABLE commit OUTPUT_STRIP_TRAILING_WHITESPACE)
      set(genavb_git_version ${branch}-${commit}${dirty})
    endif()
  endif()

  file(WRITE ${VERSION_FILE} "/* Auto-generated file. Do not edit !*/\n")
  file(APPEND ${VERSION_FILE} "#ifndef _VERSION_H_\n")
  file(APPEND ${VERSION_FILE} "#define _VERSION_H_\n\n")
  file(APPEND ${VERSION_FILE} "#define GENAVB_VERSION \"${genavb_git_version}\"\n\n")
  file(APPEND ${VERSION_FILE} "#endif /* _VERSION_H_ */\n")
  message(STATUS "generated ${VERSION_FILE} with version ${genavb_git_version}")
endfunction()
