cmake_minimum_required(VERSION 3.12...3.19)

project(xmlrpc
  DESCRIPTION "LinuxViewer xmlrpc objects."
  LANGUAGES CXX
)

include(AICxxProject)

#==============================================================================
# BUILD PROJECT
#

# This project is an OBJECT-library, used by other git submodules and the main project.
add_library(xmlrpc_ObjLib OBJECT)

# Set SOURCE_DIR to the current directory.
set_property(
  TARGET xmlrpc_ObjLib
  PROPERTY SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
)

# The list of source files.
target_sources(xmlrpc_ObjLib
  PRIVATE
    "LoginResponse.h"
)

# Required include search-paths.
get_target_property(CWDS_INTERFACE_INCLUDE_DIRECTORIES AICxx::cwds INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(xmlrpc_ObjLib
  PUBLIC
    "${CWDS_INTERFACE_INCLUDE_DIRECTORIES}" # For sys.h and debug.h.
)

# Require support for C++17.
target_compile_features(xmlrpc_ObjLib
  PUBLIC cxx_std_17
)

# Set link dependencies.
target_link_libraries(xmlrpc_ObjLib
  #  PRIVATE
  #  AICxx::utils
)

# Create an ALIAS target.
add_library(LinuxViewer::xmlrpc ALIAS xmlrpc_ObjLib)

