cmake_minimum_required(VERSION 3.12...3.19)

project(data_types
  DESCRIPTION "LinuxViewer protocol data_types."
  LANGUAGES CXX
)

include(AICxxProject)

#==============================================================================
# DEPENDENCIES
#

find_package(Boost REQUIRED COMPONENTS)

#==============================================================================
# BUILD PROJECT
#

# This project is an OBJECT-library, used by other git submodules and the main project.
add_library(data_types_ObjLib OBJECT)

# Set SOURCE_DIR to the current directory.
set_property(
  TARGET data_types_ObjLib
  PROPERTY SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
)

# The list of source files.
target_sources(data_types_ObjLib
  PRIVATE
    "BinaryData.cxx"
    "BinaryData.h"
    "UUID.cxx"
    "UUID.h"
    "DateTime.cxx"
    "DateTime.h"
)

# Required include search-paths.
get_target_property(CWDS_INTERFACE_INCLUDE_DIRECTORIES AICxx::cwds INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(data_types_ObjLib
  PUBLIC
    "${CWDS_INTERFACE_INCLUDE_DIRECTORIES}" # For sys.h and debug.h.
)

# Require support for C++17.
target_compile_features(data_types_ObjLib
  PUBLIC cxx_std_17
)

# Set link dependencies.
target_link_libraries(data_types_ObjLib
  PRIVATE
    AICxx::utils
)

# Create an ALIAS target.
add_library(LinuxViewer::date_types ALIAS data_types_ObjLib)

add_executable(base64_test EXCLUDE_FROM_ALL base64_test.cxx)
target_link_libraries(base64_test PRIVATE LinuxViewer::date_types AICxx::utils AICxx::cwds)

add_executable(uuid_test EXCLUDE_FROM_ALL uuid_test.cxx)
target_link_libraries(uuid_test PRIVATE LinuxViewer::date_types AICxx::utils AICxx::cwds)

add_executable(datetime_test EXCLUDE_FROM_ALL datetime_test.cxx)
target_link_libraries(datetime_test PRIVATE LinuxViewer::date_types AICxx::utils AICxx::cwds)
