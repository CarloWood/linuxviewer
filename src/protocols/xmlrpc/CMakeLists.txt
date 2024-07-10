cmake_minimum_required(VERSION 3.15...3.30)

project(xmlrpc
  DESCRIPTION "LinuxViewer xmlrpc objects."
  LANGUAGES CXX
)

include(AICxxProject)

#==============================================================================
# BUILD PROJECT
#

# This project is an OBJECT-library, used by other git submodules and the main project.
add_library(protocols_xmlrpc_ObjLib OBJECT)

# The list of source files.
target_sources(protocols_xmlrpc_ObjLib
  PRIVATE
    "initialize.cxx"
    "initialize.h"
    "request/LoginToSimulator.cxx"
    "request/LoginToSimulator.h"
    "response/LoginResponse.cxx"
    "response/LoginResponse.h"
)

# Required include search-paths.
get_target_property(CWDS_INTERFACE_INCLUDE_DIRECTORIES AICxx::cwds INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(protocols_xmlrpc_ObjLib
  PUBLIC
    "${CWDS_INTERFACE_INCLUDE_DIRECTORIES}" # For sys.h and debug.h.
    "${CMAKE_SOURCE_DIR}/src"
)

# Require support for C++17.
target_compile_features(protocols_xmlrpc_ObjLib
  PUBLIC cxx_std_17
)

# Set link dependencies.
target_link_libraries(protocols_xmlrpc_ObjLib
  PRIVATE
    AICxx::utils
    blaze::blaze
    magic_enum::magic_enum
)

# Create an ALIAS target.
add_library(LinuxViewer::xmlrpc ALIAS protocols_xmlrpc_ObjLib)

