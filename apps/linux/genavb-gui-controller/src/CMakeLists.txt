cmake_minimum_required(VERSION 3.10)

# OE_QMAKE_PATH_EXTERNAL_HOST_BINS needed for Qt5 toolchain config files
if(NOT DEFINED OE_QMAKE_PATH_EXTERNAL_HOST_BINS AND DEFINED ENV{OE_QMAKE_PATH_EXTERNAL_HOST_BINS})
  set(OE_QMAKE_PATH_EXTERNAL_HOST_BINS $ENV{OE_QMAKE_PATH_EXTERNAL_HOST_BINS})
  message(STATUS "Setting OE_QMAKE_PATH_EXTERNAL_HOST_BINS from environment: ${OE_QMAKE_PATH_EXTERNAL_HOST_BINS}")
endif()

# Needed when building from yocto recipe with Qt6
if(NOT DEFINED QT_HOST_PATH AND DEFINED ENV{QT_HOST_PATH})
  set(QT_HOST_PATH $ENV{QT_HOST_PATH})
  set(QT_BUILD_TOOLS_WHEN_CROSSCOMPILING ON)
  message(STATUS "Setting QT_HOST_PATH from environment: ${QT_HOST_PATH}")
endif()

project(genavb-gui-controller)

option(WAYLAND_BACKEND "Build application with support for wayland backend" ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

include_directories(${GENAVB_INCLUDE_DIR})

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets)

message(STATUS "Qt version: ${QT_VERSION}")

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
  Qt${QT_VERSION_MAJOR}::Core
  Qt${QT_VERSION_MAJOR}::Gui
  Qt${QT_VERSION_MAJOR}::Widgets
  genavb
)

# Resources:
set(gui_elements_resource_files
  "resources/developpers_Play.png"
  "resources/developpers_Stop.png"
)

set(qt_ressources
  ${PROJECT_NAME} "gui_elements"
  PREFIX "/resources"
  FILES ${gui_elements_resource_files}
)
if(${QT_VERSION} VERSION_LESS "5.15.0")
  qt5_add_resources(${qt_ressources})
else()
  qt_add_resources(${qt_ressources})
endif()

install(TARGETS ${PROJECT_NAME} DESTINATION usr/bin)
