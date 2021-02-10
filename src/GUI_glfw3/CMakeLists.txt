cmake_minimum_required(VERSION 3.12...3.19)

project(glfw3gui
  DESCRIPTION "LinuxViewer GUI implementation on top of glfw3."
  LANGUAGES CXX
)

include(AICxxProject)

#==============================================================================
# DEPENDENCIES
#

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)    # Link with glfw (it has no ALIAS).

#==============================================================================
# BUILD PROJECT
#

# This project is an OBJECT-library, used by other git submodules and the main project.
add_library(glfw3gui_ObjLib OBJECT)

# Set SOURCE_DIR to the current directory.
set_property(
  TARGET glfw3gui_ObjLib
  PROPERTY SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
)

# The list of source files.
target_sources(glfw3gui_ObjLib
  PRIVATE
    "gui_Application.cxx"
    "gui_Application.h"
    "gui_Window.cxx"
    "gui_Window.h"
    "gui_MenuEntryKey.h"
)

# Required include search-paths.
get_target_property(CWDS_INTERFACE_INCLUDE_DIRECTORIES AICxx::cwds INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(glfw3gui_ObjLib
  PUBLIC
    "${CWDS_INTERFACE_INCLUDE_DIRECTORIES}" # For sys.h and debug.h.
    "${CMAKE_SOURCE_DIR}/src"               # For LinuxViewerMenuBar.h.
)

# Require support for C++17.
target_compile_features(glfw3gui_ObjLib
  PUBLIC cxx_std_17
)

# Set link dependencies.
target_link_libraries(glfw3gui_ObjLib
  PUBLIC
    AICxx::utils
    Vulkan::Vulkan
    glfw
)

# Create an ALIAS target.
add_library(LinuxViewer::glfw3gui ALIAS glfw3gui_ObjLib)

# Has support for glfw3 and vulkan been installed on this machine?
# See https://vulkan-tutorial.com/en/Development_environment
add_executable(glfw3_test glfw3_test.cxx)

target_link_libraries(glfw3_test
  PRIVATE
    LinuxViewer::glfw3gui
)