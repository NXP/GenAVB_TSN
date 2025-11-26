include(${TARGET_OS}/extensions.cmake)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -iquote ${TOPDIR}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -iquote ${TOPDIR}/${TARGET_OS}/include")

include_directories("${TOPDIR}/include")
include_directories("${TOPDIR}/include/${TARGET_OS}")

if(CONFIG_GENAVB_TSN_API)
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

  # Force build as non PIE code and propagate the option to all its dependants.
  target_compile_options(${ARG_NAME} PUBLIC -fno-pie)

  add_custom_command(TARGET ${ARG_NAME} POST_BUILD COMMAND
    ${CMAKE_OBJCOPY} $<TARGET_FILE:${ARG_NAME}>
    --rename-section .text=.text.${ARG_NAME}
    --rename-section .data=.data.${ARG_NAME}
    --rename-section .rodata=.rodata.${ARG_NAME}
    --rename-section .bss=.bss.${ARG_NAME}
    )
endfunction()

# genavb_add_prebuilt_library(LIB <library>)
function(genavb_add_prebuilt_library)
  cmake_parse_arguments(ARG "" "LIB" "" ${ARGN})
  if(NOT DEFINED ARG_LIB)
    return()
  endif()

  if(NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/${ARG_LIB})
    message(FATAL_ERROR "Prebuilt library ${ARG_LIB} does not exist at ${CMAKE_CURRENT_LIST_DIR}")
  endif()

  message(STATUS "Importing prebuilt static library ${ARG_LIB}")

  add_library(${ARG_LIB} STATIC IMPORTED)
  set_property(TARGET ${ARG_LIB} PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_LIST_DIR}/${ARG_LIB})

  # All dependants on static prebuilt archives should compile as non-PIE code.
  target_compile_options(${ARG_LIB} INTERFACE -fno-pie)

endfunction()

# genavb_link_libraries(TARGET <target> LIB <library>)
function(genavb_link_libraries)
  cmake_parse_arguments(ARG "" "TARGET" "LIB" ${ARGN})
  if(NOT DEFINED ARG_TARGET OR NOT TARGET ${ARG_TARGET})
    return()
  endif()

  target_link_libraries(${ARG_TARGET} PUBLIC ${ARG_LIB})

  # Propagate the usage requirements of the target (stack component archive) to all dependants:
  # link as non position-independant code
  target_link_libraries(${ARG_LIB} INTERFACE -no-pie)

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

# genavb_target_add_compile_definitions(TARGET <target> DEFS <def1 def2 ...>)
function(genavb_target_add_compile_definitions)
  cmake_parse_arguments(ARG "" "TARGET" "DEFS" ${ARGN})
  if(NOT DEFINED ARG_TARGET OR NOT TARGET ${ARG_TARGET})
    return()
  endif()

  foreach(def IN LISTS ARG_DEFS)
    target_compile_definitions(${ARG_TARGET} PRIVATE ${def})
  endforeach()
endfunction()

# genavb_target_add_linker_script(TARGET <target> LINKER_SCRIPT <script>)
function(genavb_target_add_linker_script)
  cmake_parse_arguments(ARG "" "TARGET" "LINKER_SCRIPT" ${ARGN})
  if(NOT DEFINED ARG_TARGET OR NOT TARGET ${ARG_TARGET})
    return()
  endif()

  if (NOT EXISTS ${ARG_LINKER_SCRIPT})
    message(FATAL_ERROR "cannot find linker script ${ARG_LINKER_SCRIPT}")
  endif()

  set_target_properties(${ARG_TARGET} PROPERTIES LINK_DEPENDS ${ARG_LINKER_SCRIPT})
  set_property(TARGET ${ARG_TARGET} APPEND_STRING PROPERTY LINK_FLAGS " -Wl,--version-script=${ARG_LINKER_SCRIPT}")
endfunction()

function(genavb_conditional_include file)
  if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/${file})
    include(${CMAKE_CURRENT_LIST_DIR}/${file})
  endif()
endfunction()

function(genavb_include_os file)
  include(${CMAKE_CURRENT_LIST_DIR}/${file})
endfunction()
