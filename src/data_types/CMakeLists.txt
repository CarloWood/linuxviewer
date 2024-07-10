cmake_minimum_required(VERSION 3.15...3.30)

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

# The list of source files.
target_sources(data_types_ObjLib
  PRIVATE
    "UUID.cxx"
    "UUID.h"
    "URI.cxx"
    "URI.h"
    "Vector3d.cxx"
    "Vector3d.h"
    "LoginFlags.cxx"
    "LoginFlags.h"
    "InitialOutfit.cxx"
    "InitialOutfit.h"
    "AssetIdItemIdPair.cxx"
    "AssetIdItemIdPair.h"
    "Buddy.cxx"
    "Buddy.h"
    "Category.cxx"
    "Category.h"
    "SunCloudMoonTextures.cxx"
    "SunCloudMoonTextures.h"
    "RegionPositionLookAt.cxx"
    "RegionPositionLookAt.h"
    "RegionHandle.cxx"
    "RegionHandle.h"
    "Position.cxx"
    "Position.h"
    "LookAt.cxx"
    "LookAt.h"
    "AgentID.cxx"
    "AgentID.h"
    "FolderID.cxx"
    "FolderID.h"
    "InventoryFolder.cxx"
    "InventoryFolder.h"
    "UIConfig.cxx"
    "UIConfig.h"
    "AgentAccess.cxx"
    "AgentAccess.h"
    "Gender.cxx"
    "Gender.h"
    "ModerationLevel.cxx"
    "ModerationLevel.h"
)

# Required include search-paths.
get_target_property(CWDS_INTERFACE_INCLUDE_DIRECTORIES AICxx::cwds INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(data_types_ObjLib
  PUBLIC
    "${CWDS_INTERFACE_INCLUDE_DIRECTORIES}" # For sys.h and debug.h.
    "${CMAKE_SOURCE_DIR}/src"
)

# Require support for C++17.
target_compile_features(data_types_ObjLib
  PUBLIC cxx_std_17
)

# Set link dependencies.
target_link_libraries(data_types_ObjLib
  PRIVATE
    AICxx::utils
  PUBLIC
    blaze::blaze
    magic_enum::magic_enum
)

# Create an ALIAS target.
add_library(LinuxViewer::data_types ALIAS data_types_ObjLib)

add_executable(base64_test EXCLUDE_FROM_ALL base64_test.cxx)
target_link_libraries(base64_test PRIVATE LinuxViewer::data_types AICxx::evio AICxx::evio_protocol_xmlrpc AICxx::evio_protocol AICxx::threadpool AICxx::utils AICxx::cwds)

add_executable(uuid_test EXCLUDE_FROM_ALL uuid_test.cxx)
target_link_libraries(uuid_test PRIVATE LinuxViewer::data_types AICxx::evio AICxx::evio_protocol_xmlrpc AICxx::evio_protocol AICxx::threadpool AICxx::utils AICxx::cwds)

add_executable(datetime_test EXCLUDE_FROM_ALL datetime_test.cxx)
target_link_libraries(datetime_test PRIVATE LinuxViewer::data_types AICxx::evio AICxx::evio_protocol_xmlrpc AICxx::evio_protocol AICxx::threadpool AICxx::utils AICxx::cwds)

add_executable(uri_test EXCLUDE_FROM_ALL uri_test.cxx)
target_link_libraries(uri_test PRIVATE LinuxViewer::data_types AICxx::evio AICxx::evio_protocol_xmlrpc AICxx::evio_protocol AICxx::threadpool AICxx::utils AICxx::cwds)

add_executable(blaze_test EXCLUDE_FROM_ALL blaze_test.cxx)
target_link_libraries(blaze_test PRIVATE LinuxViewer::data_types AICxx::evio AICxx::evio_protocol_xmlrpc AICxx::evio_protocol AICxx::threadpool AICxx::utils AICxx::cwds)
