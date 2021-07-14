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
    "gui_WindowCreateInfo.cxx"
    "gui_WindowCreateInfo.h"
)

# Required include search-paths.
get_target_property(CWDS_INTERFACE_INCLUDE_DIRECTORIES AICxx::cwds INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(glfw3gui_ObjLib
  PUBLIC
    "${CWDS_INTERFACE_INCLUDE_DIRECTORIES}" # For sys.h and debug.h.
    "${CMAKE_SOURCE_DIR}/src"               # For LinuxViewerMenuBar.h.
)

target_compile_definitions(glfw3gui_ObjLib
  PUBLIC
    GLFW_INCLUDE_VULKAN
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
    GLFWPP
)

# Create an ALIAS target.
add_library(LinuxViewer::glfw3gui ALIAS glfw3gui_ObjLib)

# C++ wrapper around glfw. Also includes external "dependencies" for imgui and glew (with cmake support).
add_subdirectory(glfwpp)
