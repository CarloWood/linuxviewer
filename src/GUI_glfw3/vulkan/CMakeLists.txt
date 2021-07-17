cmake_minimum_required(VERSION 3.12...3.19)

include(AICxxProject)

find_package(Vulkan REQUIRED COMPONENTS glslc)
find_program(glslc_executable NAMES glslc HINTS Vulkan::glslc)

#==============================================================================
# COMPILE SHADERS
#

set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)

file(GLOB SHADERS
  ${SHADER_DIR}/*.vert
  ${SHADER_DIR}/*.frag
  ${SHADER_DIR}/*.comp
  ${SHADER_DIR}/*.geom
  ${SHADER_DIR}/*.tesc
  ${SHADER_DIR}/*.tese
  ${SHADER_DIR}/*.mesh
  ${SHADER_DIR}/*.task
  ${SHADER_DIR}/*.rgen
  ${SHADER_DIR}/*.rchit
  ${SHADER_DIR}/*.rmiss)

foreach(source IN LISTS SHADERS)
  get_filename_component(FILENAME ${source} NAME)
  add_custom_command(
    COMMAND
      ${glslc_executable}
      -MD -MF ${source}.d
      -o ${source}.spv
      ${source}
    OUTPUT ${SHADER_DIR}/${FILENAME}.spv
    DEPENDS ${source}
    COMMENT "Compiling ${FILENAME}"
  )
  list(APPEND SPV_SHADERS ${SHADER_DIR}/${FILENAME}.spv)
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
    "HelloTriangleDevice.cxx"
    "HelloTriangleDevice.h"
    "PipelineCreateInfo.cxx"
    "PipelineCreateInfo.h"
    "Pipeline.cxx"
    "Pipeline.h"
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
    SHADER_DIR="${SHADER_DIR}"
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

# Create an ALIAS target.
add_library(LinuxViewer::vulkan ALIAS vulkan_ObjLib)
