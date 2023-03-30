# FetchContent was added to 3.14.
cmake_minimum_required(VERSION 3.14...3.17)

# Compile all subdirectories with the same standard.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(linuxviewer
  LANGUAGES C CXX
  DESCRIPTION "Viewer for SecondLife and OpenSim."
)

#==============================================================================
# Begin of gitache configuration.

set(GITACHE_PACKAGES magic_enum blaze libcwd_r wolfssl)

# Is libfarmhash installed somewhere in a default path?
find_library(
  FARMHASH farmhash
  DOC "Whether or not libfarmhash is installed locally."
)
# If not, use gitache to get it.
if(NOT FARMHASH)
  list(APPEND GITACHE_PACKAGES farmhash)
endif()

# Is shaderc installed on the system?
find_package(PkgConfig REQUIRED)
pkg_check_modules(Shaderc IMPORTED_TARGET shaderc)

if (NOT Shaderc_FOUND)
  message(STATUS "Adding shaderc to gitache packages.")
  list(APPEND GITACHE_PACKAGES shaderc)
endif ()

#include(gitache/gateway.cmake)
# OR
include(FetchContent)

# If a local gitache submodule is present then use that rather than downloading one.
if (EXISTS ${CMAKE_CURRENT_LIST_DIR}/gitache/.git)
  # This will disable the use of the GIT_REPOSITORY/GIT_TAG below, and disable the
  # FetchContent- download and update step. Instead, use the gitache submodule as-is.
  set(FETCHCONTENT_SOURCE_DIR_GITACHE "${CMAKE_CURRENT_LIST_DIR}/gitache" CACHE INTERNAL "" FORCE)
endif ()

FetchContent_Declare(
  gitache
  GIT_REPOSITORY "https://github.com/CarloWood/gitache.git"
  GIT_TAG        "stable"       # Normally use either "stable" or a fixed SHA1 to freeze the version here!
                                # Using "master" might be *unstable*.
)
FetchContent_MakeAvailable(gitache)
# End of gitache configuration.

#==============================================================================
# This project uses aicxx modules.
include(cwm4/cmake/AICxxProject)

#==============================================================================
# OPTIONS

if (CW_BUILD_TYPE_IS_TRACY)
  set(CW_BUILD_TYPE_IS_NOT_TRACY FALSE)
  set(TRACY_ENABLE 1)
  set(SetThreadName_module threadpool)
else ()
  # Enable the option below, so people can override linking with Tracy anyway.
  set(CW_BUILD_TYPE_IS_NOT_TRACY TRUE)
endif ()

# Option 'EnableTracy' is only available when both CW_BUILD_TYPE_IS_NOT_TRACY
# and CW_BUILD_TYPE_IS_NOT_RELEASE are TRUE (otherwise OFF).
cw_option(EnableTracy
          "Enable profiling code using Tracy" OFF
          "CW_BUILD_TYPE_IS_NOT_TRACY;CW_BUILD_TYPE_IS_NOT_RELEASE" OFF
)

if (OptionEnableTracy)
  message(DEBUG "OptionEnableTracy is ${OptionEnableTracy}")
  set(TRACY_ENABLE 1)
  if (OptionEnableLibcwd)
    set(SetThreadName_module cwds)
  else ()
    set(SetThreadName_module threadpool)
  endif ()
endif ()
# End of OPTIONS section.

#==============================================================================

# Exit if someone tries to contaminate the source directory with an in-source build.
if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "Please do out-of-source builds."
                      "Cleanup: \"rm -rf CMake CMakeCache.txt CMakeFiles/\"")
endif ()

#==============================================================================
# CONFIG FILES

# Tell cwds/sys.h that we have a config.h.
#add_definitions(-DHAVE_CONFIG_H)
#include_directories(${top_objdir})
# End of CONFIG FILES section.

#==============================================================================

# Declare aicxx submodules.
include(AICxxSubmodules)

# This must be done after including AICxxSubmodules.
if (SetThreadName_module)
# Link with libTracyClient.a (needed for tracy::SetThreadName).
target_link_libraries(${SetThreadName_module}_ObjLib
  PRIVATE
    Tracy::TracyClient
)
endif ()

# Extra clean up before removing directories.
add_custom_target(maintainer-clean-extra
  COMMAND rm -f CMakeDoxygenDefaults.cmake CMakeDoxyfile.in
)

# Do not build the googlemock subproject.
set(BUILD_GMOCK OFF CACHE BOOL "")
# Disable installation of googletest.
set(INSTALL_GTEST OFF CACHE BOOL "")

add_subdirectory(docs)
add_subdirectory(external)
#add_subdirectory(helloworld-task)
#add_subdirectory(filelock-task)
add_subdirectory(resolver-task)
add_subdirectory(socket-task)
add_subdirectory(xmlrpc-task)
add_subdirectory(dbus-task)
add_subdirectory(xcb-task)
add_subdirectory(block-task)

add_subdirectory(src)
