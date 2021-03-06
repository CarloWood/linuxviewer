include(AICxxProject)

#==============================================================================
# DEPENDENCIES
#

find_package(Boost REQUIRED COMPONENTS serialization) #filesystem iostreams program_options system)
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

add_executable(linuxviewer 
  LinuxViewerApplication.cxx
  LinuxViewerApplication.h
  LinuxViewerMenuEntries.cxx
  LinuxViewerMenuEntries.h
  ApplicationCreateInfo.cxx
  ApplicationCreateInfo.h
  WindowCreateInfo.cxx
  WindowCreateInfo.h
  Application.cxx
  Application.h

  LinuxViewerMenuBar.h
)

target_link_libraries(linuxviewer
  PRIVATE
    LinuxViewer::protocols
    LinuxViewer::xmlrpc
    LinuxViewer::data_types
    LinuxViewer::glfw3gui
    #LinuxViewer::gtkmmgui
    AICxx::socket-task
    AICxx::resolver-task
    AICxx::xmlrpc-task
    dns::dns
    ${AICXX_OBJECTS_LIST}
    farmhash::farmhash
    blaze::blaze
    magic_enum::magic_enum
    Boost::serialization
)

add_executable(hellotriangle
  HelloTriangleVulkanApplicationCreateInfo.cxx
  HelloTriangleVulkanApplicationCreateInfo.h
  HelloTriangleVulkanApplication.cxx
  HelloTriangleVulkanApplication.h
  ApplicationCreateInfo.cxx
  ApplicationCreateInfo.h
  WindowCreateInfo.cxx
  WindowCreateInfo.h
  Application.cxx
  Application.h
)

target_link_libraries(hellotriangle
  PRIVATE
    LinuxViewer::glfw3gui
    AICxx::resolver-task
    dns::dns
    ${AICXX_OBJECTS_LIST}
    farmhash::farmhash
)

#add_subdirectory(GUI_gtkmm3)
add_subdirectory(GUI_glfw3)
add_subdirectory(protocols)
add_subdirectory(data_types)
