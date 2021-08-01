cmake_minimum_required(VERSION 3.12...3.19)

include(AICxxProject)

find_package(Vulkan REQUIRED COMPONENTS glslc)
find_program(glslc_executable NAMES glslc HINTS Vulkan::glslc)

find_package(PkgConfig REQUIRED)
pkg_check_modules(Shaderc REQUIRED IMPORTED_TARGET shaderc)

#==============================================================================
# COMPILE SHADERS
#

set(SHADERS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(SHADERS_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)

file(GLOB SHADERS
  ${SHADERS_SOURCE_DIR}/*.vert
  ${SHADERS_SOURCE_DIR}/*.frag
  ${SHADERS_SOURCE_DIR}/*.comp
  ${SHADERS_SOURCE_DIR}/*.geom
  ${SHADERS_SOURCE_DIR}/*.tesc
  ${SHADERS_SOURCE_DIR}/*.tese
  ${SHADERS_SOURCE_DIR}/*.mesh
  ${SHADERS_SOURCE_DIR}/*.task
  ${SHADERS_SOURCE_DIR}/*.rgen
  ${SHADERS_SOURCE_DIR}/*.rchit
  ${SHADERS_SOURCE_DIR}/*.rmiss)

add_custom_command(
  COMMAND
    ${CMAKE_COMMAND} -E make_directory ${SHADERS_BINARY_DIR}
  OUTPUT ${SHADERS_BINARY_DIR}
  COMMENT "Creating ${SHADERS_BINARY_DIR}"
)

foreach(source IN LISTS SHADERS)
  get_filename_component(FILENAME ${source} NAME)
  add_custom_command(
    COMMAND
      ${glslc_executable}
      -o ${SHADERS_BINARY_DIR}/${FILENAME}.spv
      ${source}
    OUTPUT ${SHADERS_BINARY_DIR}/${FILENAME}.spv
    DEPENDS ${source} ${SHADERS_BINARY_DIR}
    COMMENT "Compiling ${FILENAME}"
  )
  list(APPEND SPV_SHADERS ${SHADERS_BINARY_DIR}/${FILENAME}.spv)
endforeach()

add_custom_target(shaders ALL DEPENDS ${SPV_SHADERS})

#==============================================================================
# BUILD PROJECT
#

# This project is an OBJECT-library, used by other git submodules and the main project.
add_library(vulkan_ObjLib OBJECT)

# Set SOURCE_DIR to the current directory.
set_property(
  TARGET vulkan_ObjLib
  PROPERTY SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
)

# The list of source files.
target_sources(vulkan_ObjLib
  PRIVATE
    "InstanceCreateInfo.cxx"
    "InstanceCreateInfo.h"
    "ExtensionLoader.cxx"
    "ExtensionLoader.h"
    "HelloTriangleDevice.cxx"
    "Device.cxx"
    "Device.h"
    "DebugMessenger.cxx"
    "DebugMessenger.h"
    "PipelineCreateInfo.cxx"
    "PipelineCreateInfo.h"
    "Pipeline.cxx"
    "Pipeline.h"
    "HelloTriangleSwapChain.cxx"
    "HelloTriangleSwapChain.h"
    "debug_ostream_operators.cxx"
    "debug_ostream_operators.h"
)

# Required include search-paths.
get_target_property(CWDS_INTERFACE_INCLUDE_DIRECTORIES AICxx::cwds INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(vulkan_ObjLib
  PUBLIC
    "${CWDS_INTERFACE_INCLUDE_DIRECTORIES}" # For sys.h and debug.h.
)

# Because we currently include glfwpp/glfwpp.h.
target_compile_definitions(vulkan_ObjLib
  PUBLIC
    GLFW_INCLUDE_VULKAN
    SHADERS_DIR="${SHADERS_BINARY_DIR}"
    VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
)

# Require support for C++17.
target_compile_features(vulkan_ObjLib
  PUBLIC cxx_std_17
)

# Set link dependencies.
target_link_libraries(vulkan_ObjLib
  PUBLIC
    GLFWPP
    Vulkan::Vulkan
    AICxx::cwds
)

add_executable(minimalexample
  minimal_example.cxx
)

target_compile_definitions(minimalexample
  PUBLIC
    VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
)

target_link_libraries(minimalexample
  PRIVATE
    GLFWPP
    Vulkan::Vulkan
    PkgConfig::Shaderc
)

# Create an ALIAS target.
add_library(LinuxViewer::vulkan ALIAS vulkan_ObjLib)
