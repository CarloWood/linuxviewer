cmake_minimum_required(VERSION 3.12...3.16)

project(gtkmmgui
  DESCRIPTION "LinuxViewer GUI implementation on top of gtkmm3."
  LANGUAGES CXX
)

include(AICxxProject)

#==============================================================================
# DEPENDENCIES
#

find_package(PkgConfig REQUIRED)

pkg_check_modules(gtk REQUIRED IMPORTED_TARGET gtk+-3.0)
pkg_check_modules(giomm REQUIRED IMPORTED_TARGET giomm-2.4)
pkg_check_modules(glibmm REQUIRED IMPORTED_TARGET glibmm-2.4)
pkg_check_modules(gtkmm REQUIRED IMPORTED_TARGET gtkmm-3.0)

#==============================================================================
# BUILD PROJECT
#

# This project is an OBJECT-library, used by other git submodules and the main project.
add_library(gtkmmgui_ObjLib OBJECT)

# Set SOURCE_DIR to the current directory.
set_property(
  TARGET gtkmmgui_ObjLib
  PROPERTY SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
)

# The list of source files.
target_sources(gtkmmgui_ObjLib
  PRIVATE
    "GUIApplication.cxx"
    "GUIApplication.h"
    "GUIWindow.cxx"
    "GUIWindow.h"
    "GUIIconFactory.cxx"
    "GUIIconFactory.h"
    "GUIMenuBar.cxx"
    "GUIMenuBar.h"
)

# Required include search-paths.
get_target_property(CWDS_INTERFACE_INCLUDE_DIRECTORIES AICxx::cwds INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(gtkmmgui_ObjLib
  PUBLIC
    "${CWDS_INTERFACE_INCLUDE_DIRECTORIES}" # For sys.h and debug.h.
)

# Require support for C++17.
target_compile_features(gtkmmgui_ObjLib
  PUBLIC cxx_std_17
)

# Set link dependencies.
target_link_libraries(gtkmmgui_ObjLib
  PUBLIC
    PkgConfig::gtkmm
)

# Create an ALIAS target.
add_library(LinuxViewer::gtkmmgui ALIAS gtkmmgui_ObjLib)
