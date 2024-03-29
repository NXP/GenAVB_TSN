cmake_minimum_required(VERSION 3.10)

# Program is compiled and executed on host machine
# We use custom_commands to avoid yocto toolchain cross-compile settings

project(aem-manager)

# suppress cmake warning
set(unused_var ${GENAVB_LIB_DIR})

if(NOT DEFINED GENAVB_INCLUDE_DIR)
  set(GENAVB_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../include")
  set(GENAVB_INCLUDE_OS_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../include/linux")
  set(COMPILE_FLAGS -I${GENAVB_INCLUDE_OS_DIR})
endif()
set(COMPILE_FLAGS ${COMPILE_FLAGS} -I${GENAVB_INCLUDE_DIR})

find_program(GCC "gcc" NO_CMAKE_FIND_ROOT_PATH)
if(GCC STREQUAL "GCC-NOTFOUND")
  message(FATAL_ERROR "gcc not found")
endif()

set(ENTITIES
  ${CMAKE_SOURCE_DIR}/listener_audio_single.c
  ${CMAKE_SOURCE_DIR}/listener_audio_single_milan.c
  ${CMAKE_SOURCE_DIR}/listener_talker_audio_single.c
  ${CMAKE_SOURCE_DIR}/listener_talker_audio_single_milan.c
  ${CMAKE_SOURCE_DIR}/listener_video_multi.c
  ${CMAKE_SOURCE_DIR}/listener_video_single.c
  ${CMAKE_SOURCE_DIR}/talker_audio_single.c
  ${CMAKE_SOURCE_DIR}/talker_audio_single_milan.c
  ${CMAKE_SOURCE_DIR}/talker_audio_video.c
  ${CMAKE_SOURCE_DIR}/talker_video_multi.c
  ${CMAKE_SOURCE_DIR}/talker_video_single.c
  ${CMAKE_SOURCE_DIR}/talker_listener_audio_multi.c
  ${CMAKE_SOURCE_DIR}/talker_listener_audio_multi_aaf.c
  ${CMAKE_SOURCE_DIR}/talker_listener_audio_multi_format.c
  ${CMAKE_SOURCE_DIR}/controller.c
  ${CMAKE_SOURCE_DIR}/avnu_certification.c
)

set(SOURCES
  ${ENTITIES}
  ${CMAKE_SOURCE_DIR}/main.c
  ${CMAKE_SOURCE_DIR}/../../../public/aem_helpers.c
  ${CMAKE_SOURCE_DIR}/../../../public/linux/aem_helpers.c
  ${CMAKE_SOURCE_DIR}/../../../public/helpers.c
)

set(COMPILE_FLAGS ${COMPILE_FLAGS} -O2 -Wall -Werror -g)

# compile
add_custom_command(
  OUTPUT ${PROJECT_NAME}
  COMMAND ${GCC} ${COMPILE_FLAGS} ${SOURCES} -o ${PROJECT_NAME}
  DEPENDS ${SOURCES} ${CMAKE_SOURCE_DIR}/*.h
  COMMENT "Build ${PROJECT_NAME}"
)

# once compiled, run the program to generate entities binaries
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/entities
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME} -c > ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.log
  COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/entities
  COMMAND mv ${CMAKE_CURRENT_BINARY_DIR}/*.aem ${CMAKE_CURRENT_BINARY_DIR}/entities
  COMMAND touch ${CMAKE_CURRENT_BINARY_DIR}/entities
  DEPENDS ${PROJECT_NAME}
  COMMENT "Generate aem entities"
)
add_custom_target(gen_entities ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/entities)

if(${CMAKE_VERSION} VERSION_LESS "3.15.0")
  set_property(DIRECTORY PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.log
    )
else()
  set_property(TARGET gen_entities APPEND PROPERTY ADDITIONAL_CLEAN_FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.log
    )
endif()

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/entities/ DESTINATION etc/genavb FILES_MATCHING PATTERN "*.aem")
