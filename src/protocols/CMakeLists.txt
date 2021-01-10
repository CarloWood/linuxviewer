cmake_minimum_required(VERSION 3.12...3.19)

project(protocols
  DESCRIPTION "LinuxViewer protocol decoders and related stuff."
  LANGUAGES CXX
)

include(AICxxProject)

#==============================================================================
# DEPENDENCIES
#

find_package(PkgConfig REQUIRED)

pkg_check_modules(Libxmlpp30 REQUIRED IMPORTED_TARGET libxml++-3.0)

#==============================================================================
# BUILD PROJECT
#

# This project is an OBJECT-library, used by other git submodules and the main project.
add_library(protocols_ObjLib OBJECT)

# Set SOURCE_DIR to the current directory.
set_property(
  TARGET protocols_ObjLib
  PROPERTY SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
)

# The list of source files.
target_sources(protocols_ObjLib
  PRIVATE
    "GridInfo.cxx"
    "GridInfo.h"
    "GridInfoDecoder.cxx"
    "GridInfoDecoder.h"
    "LlsdDecoder.cxx"
    "LlsdDecoder.h"
)

# Required include search-paths.
get_target_property(CWDS_INTERFACE_INCLUDE_DIRECTORIES AICxx::cwds INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(protocols_ObjLib
  PUBLIC
    "${CWDS_INTERFACE_INCLUDE_DIRECTORIES}" # For sys.h and debug.h.
)

# Require support for C++17.
target_compile_features(protocols_ObjLib
  PUBLIC cxx_std_17
)

# Set link dependencies.
target_link_libraries(protocols_ObjLib
  PRIVATE
    AICxx::utils
  PUBLIC
    PkgConfig::Libxmlpp30
)

# Create an ALIAS target.
add_library(LinuxViewer::protocols ALIAS protocols_ObjLib)

add_executable(test_llsd_decoder_end_of_msg_finder
  "test_llsd_decoder_end_of_msg_finder.cxx"
)

target_link_libraries(test_llsd_decoder_end_of_msg_finder
  PRIVATE
    ${AICXX_OBJECTS_LIST}
    LinuxViewer::protocols
)
