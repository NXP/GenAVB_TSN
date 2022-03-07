cmake_minimum_required(VERSION 3.10)

project(genavb-gui-controller)

option(WAYLAND_BACKEND "Build application with support for wayland backend" ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

include_directories(${GENAVB_INCLUDE_DIR})

find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets)

add_executable(${PROJECT_NAME}
  main.cpp
  mainform.ui
  avb.c
  window.cpp
  avbcontroller.cpp
  avbinputeventlistener.cpp
  ../../common/aecp.c
  ../../common/common.c
  ../../common/stats.c
  ../../common/time.c
)

if(WAYLAND_BACKEND)
  target_compile_definitions(${PROJECT_NAME} PUBLIC WL_BUILD)
endif()

if(DEFINED GENAVB_LIB_DIR)
  add_library(genavb SHARED IMPORTED)
  set_target_properties(genavb PROPERTIES IMPORTED_LOCATION "${GENAVB_LIB_DIR}/libgenavb.so")
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC
  Qt5::Core
  Qt5::Gui
  Qt5::Widgets
  genavb
)

# Resources:
set(gui_elements_resource_files
  "resources/developpers_Play.png"
  "resources/developpers_Stop.png"
)

qt5_add_resources(${PROJECT_NAME} "gui_elements"
  PREFIX "/resources"
  FILES ${gui_elements_resource_files}
)

install(TARGETS ${PROJECT_NAME} DESTINATION usr/bin)
