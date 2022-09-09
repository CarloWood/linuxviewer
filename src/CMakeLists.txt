include(AICxxProject)

#==============================================================================
# DEPENDENCIES
#

find_package(farmhash REQUIRED)
find_package(blaze REQUIRED)
find_package(magic_enum REQUIRED)

# Use --disable-new-dtags when linking executables. This is to avoid setting the RUNPATH
# entry in the dynamic section of executables when using -rpath. As a result the path
# set with -rpath takes precedence over LD_LIBRARY_PATH.
#
# In general, the CORRECT way to deal with this is to make sure that you do not have
# libraries installed in your LD_LIBRARY_PATH that you don't want to link with (aka,
# that are installed in gitache!).
###set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--disable-new-dtags")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)

#==============================================================================

add_compile_definitions(
  IMGUI_USER_CONFIG="lvimconfig.h"
)

add_executable(linuxviewer 
  LinuxViewerApplication.cxx
  LinuxViewerApplication.h
  LinuxViewerMenuEntries.cxx
  LinuxViewerMenuEntries.h
  LinuxViewerMenuBar.h
)

target_link_libraries(linuxviewer
  PRIVATE
    LinuxViewer::protocols
    LinuxViewer::xmlrpc
    LinuxViewer::data_types
    LinuxViewer::vulkan
    LinuxViewer::shader_builder
    AICxx::socket-task
    AICxx::resolver-task
    AICxx::xmlrpc-task
    AICxx::xcb-task
    AICxx::xcb-task::OrgFreedesktopXcbError
    ImGui::imgui
    ${AICXX_OBJECTS_LIST}
    dns::dns
    farmhash::farmhash
    blaze::blaze
)

add_subdirectory(protocols)
add_subdirectory(data_types)

# Vulkan stuff.
add_subdirectory(vulkan)

# Test applications.
add_subdirectory(tests)
