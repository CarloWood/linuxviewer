cmake_minimum_required(VERSION 3.15...3.30)

include(AICxxProject)

find_package(Eigen3 3.3 REQUIRED NO_MODULE)
find_package(magic_enum REQUIRED)

#==============================================================================
# BUILD PROJECT
#

add_library(shader_builder_ObjLib OBJECT)

file(GLOB SOURCE_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/shader_resource/*.cxx
)

file(GLOB HEADER_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/shader_resource/*.h
)

# The list of source files.
target_sources(shader_builder_ObjLib
  PRIVATE
    ${SOURCE_FILES}
    ${HEADER_FILES}
)

# Required include search-paths.
get_target_property(CWDS_INTERFACE_INCLUDE_DIRECTORIES AICxx::cwds INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(shader_builder_ObjLib
  PRIVATE
    "${CMAKE_SOURCE_DIR}/src/vulkan"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CWDS_INTERFACE_INCLUDE_DIRECTORIES}" # For sys.h and debug.h.
)

target_compile_definitions(shader_builder_ObjLib
  PRIVATE
    LV_COMPILING_VULKAN_ENGINE
  PUBLIC
    VK_USE_PLATFORM_XCB_KHR
    VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
    VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
    VULKAN_HPP_NO_UNION_CONSTRUCTORS
)

# Require support for C++20.
target_compile_features(shader_builder_ObjLib
  PUBLIC cxx_std_20
)

# Set link dependencies.
target_link_libraries(shader_builder_ObjLib
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
add_library(LinuxViewer::shader_builder ALIAS shader_builder_ObjLib)
