cmake_minimum_required(VERSION 3.15...3.30)

project(protocols
  DESCRIPTION "LinuxViewer protocol decoders and related stuff."
  LANGUAGES CXX
)

include(AICxxProject)

#==============================================================================
# DEPENDENCIES
#

find_package(PkgConfig REQUIRED)

pkg_check_modules(Libxmlpp30 libxml++-3.0 IMPORTED_TARGET)
if (Libxmlpp30_FOUND)
  set(LIBXMLPP PkgConfig::Libxmlpp30)
  set(LIBXMLPP_VERSION 30)
else ()
  pkg_check_modules(Libxmlpp26 libxml++-2.6 IMPORTED_TARGET)
  if (Libxmlpp26_FOUND)
    set(LIBXMLPP PkgConfig::Libxmlpp26)
    set(LIBXMLPP_VERSION 26)
  else ()
    message(FATAL_ERROR "Neither libxml++-3.0 nor libxml++-2.6 could be found!")
  endif ()
endif ()

#==============================================================================
# BUILD PROJECT
#

# This project is an OBJECT-library, used by other git submodules and the main project.
add_library(protocols_ObjLib OBJECT)

# The list of source files.
target_sources(protocols_ObjLib
  PRIVATE
    "GridInfo.cxx"
    "GridInfo.h"
    "GridInfoDecoder.cxx"
    "GridInfoDecoder.h"
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
    blaze::blaze
    ${LIBXMLPP}
)

# Create an ALIAS target.
add_library(LinuxViewer::protocols ALIAS protocols_ObjLib)

add_subdirectory(xmlrpc)
