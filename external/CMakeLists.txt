message(STATUS "----------------------------------------------------\n** Configuring subdirectory external:")

# imgui is weird.
# It doesn't want you to build it using cmake. You have to pick the files you need yourself.

cmake_minimum_required(VERSION 3.12...3.16)

project(imgui_ObjLib
  DESCRIPTION "Immediate Mode Graphical User Interface"
  LANGUAGES CXX
)

#==============================================================================
# BUILD PROJECT
#

add_compile_definitions(
  IMGUI_USER_CONFIG="lvimconfig.h"
)

# This project is an OBJECT-library, used by the main project.
add_library(imgui_ObjLib OBJECT)

# The list of source files.
target_sources(imgui_ObjLib
  PRIVATE
    # ImGui
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_demo.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_draw.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_widgets.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_tables.cpp"
    # Renderer
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_vulkan.cpp"
    # Header
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.h"
)

target_include_directories(imgui_ObjLib
  PUBLIC
    # Because the .cpp files use #include "xyz.h", expecting the source directory to be "${CMAKE_CURRENT_SOURCE_DIR}/imgui".
    # And when using imgui, we can't use #include <imgui/*> because backends/*.h use #include "imgui.h"; so we'll just
    # have to include this directory at all times.
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui"
  PRIVATE
    # In order to find lvimconfig.h while compiling imgui.
    "${CMAKE_SOURCE_DIR}"
)

# This is needed to add debug output to Dear ImGui itself (branch libcwd).
# It is not necessary for imgui's master branch - but shouldn't hurt either.
if (OptionEnableLibcwd)
  target_link_libraries(imgui_ObjLib
    PRIVATE
      ${libcwd_r_TARGET}
)
endif ()

# Create an ALIAS target.
add_library(ImGui::imgui ALIAS imgui_ObjLib)

# Create target VulkanMemoryAllocator.
set(VMA_STATIC_VULKAN_FUNCTIONS OFF CACHE BOOL "Link statically with Vulkan API")
set(VMA_DYNAMIC_VULKAN_FUNCTIONS ON CACHE BOOL "Fetch pointers to Vulkan functions internally (no static linking)")
add_subdirectory(VulkanMemoryAllocator)

# Add Tracy.
if (TRACY_ENABLE)
  # Add other options conditionally, because they'll clutter command line defines even when TRACY_ENABLE is OFF.
  set(TRACY_ENABLE ON CACHE BOOL "Enable profiling")
  set(TRACY_ONLY_LOCALHOST ON CACHE BOOL "Only listen on the localhost interface")
  set(TRACY_FIBERS ON CACHE BOOL "Enable fibers")
  set(TRACY_NO_CALLSTACK ON CACHE BOOL "No callstack retrieval") # If using callstacks macro's, turn this OFF and compile with -g -fno-omit-frame-pointer.
else ()
  # Must set TRACY_ENABLE explicitly to OFF; the default is ON.
  set(TRACY_ENABLE OFF CACHE BOOL "Enable profiling")
endif ()
add_subdirectory(tracy)
