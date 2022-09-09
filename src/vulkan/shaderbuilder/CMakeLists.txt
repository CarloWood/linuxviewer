cmake_minimum_required(VERSION 3.12...3.19)

include(AICxxProject)

find_package(Eigen3 3.3 REQUIRED NO_MODULE)
find_package(magic_enum REQUIRED)

#==============================================================================
# BUILD PROJECT
#

add_library(shaderbuilder_ObjLib OBJECT)

# Set SOURCE_DIR to the current directory.
set_property(
  TARGET shaderbuilder_ObjLib
  PROPERTY SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
)

file(GLOB SOURCE_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/shader_resource/*.cxx
)

file(GLOB HEADER_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/shader_resource/*.h
)

# The list of source files.
target_sources(shaderbuilder_ObjLib
  PRIVATE
    ${SOURCE_FILES}
    ${HEADER_FILES}
)

# Required include search-paths.
get_target_property(CWDS_INTERFACE_INCLUDE_DIRECTORIES AICxx::cwds INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(shaderbuilder_ObjLib
  PRIVATE
    "${CMAKE_SOURCE_DIR}/src/vulkan"
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CWDS_INTERFACE_INCLUDE_DIRECTORIES}" # For sys.h and debug.h.
)

target_compile_definitions(shaderbuilder_ObjLib
  PUBLIC
    VK_USE_PLATFORM_XCB_KHR
    VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
    VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
    VULKAN_HPP_NO_UNION_CONSTRUCTORS
)

# Require support for C++20.
target_compile_features(shaderbuilder_ObjLib
  PUBLIC cxx_std_20
)

# Set link dependencies.
target_link_libraries(shaderbuilder_ObjLib
  PUBLIC
    VulkanMemoryAllocator
    Vulkan::Vulkan
    AICxx::utils
    farmhash::farmhash
    Eigen3::Eigen
    magic_enum::magic_enum
    Tracy::TracyClient
)

# Create an ALIAS target.
add_library(LinuxViewer::shaderbuilder ALIAS shaderbuilder_ObjLib)
