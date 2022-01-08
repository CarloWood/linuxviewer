# imgui is weird.
# It doesn't want you to build it using cmake. You have to pick the files you need yourself.

cmake_minimum_required(VERSION 3.12...3.16)

project(imgui_ObjLib
  DESCRIPTION "Immediate Mode Graphical User Interface"
  LANGUAGES CXX
)

#==============================================================================
# BUILD PROJECT
#

# This project is an OBJECT-library, used by the main project.
add_library(imgui_ObjLib OBJECT)

# The list of source files.
target_sources(imgui_ObjLib
  PRIVATE
    # ImGui
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_demo.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_draw.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_widgets.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_tables.cpp"
    # Renderer
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_vulkan.cpp"
    # Header
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.h"
)

target_include_directories(imgui_ObjLib
  PUBLIC
    # Because the .cpp files use #include "xyz.h", expecting the source directory to be "${CMAKE_CURRENT_SOURCE_DIR}/imgui".
    # And when using imgui, we can't use #include <imgui/*> because backends/*.h use #include "imgui.h"; so we'll just
    # have to include this directory at all times.
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui"
)

# Create an ALIAS target.
add_library(ImGui::imgui ALIAS imgui_ObjLib)
