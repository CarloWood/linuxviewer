include(AICxxProject)

#find_package(Threads REQUIRED)
#find_package(Boost REQUIRED COMPONENTS filesystem iostreams program_options system)
find_package(farmhash REQUIRED)
find_package(blaze REQUIRED)

# Use --disable-new-dtags when linking executables. This is to avoid setting the RUNPATH
# entry in the dynamic section of executables when using -rpath. As a result the path
# set with -rpath takes precedence over LD_LIBRARY_PATH.
#
# In general, the CORRECT way to deal with this is to make sure that you do not have
# libraries installed in your LD_LIBRARY_PATH that you don't want to link with (aka,
# that are installed in gitache!).
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--disable-new-dtags")

add_executable(linuxviewer 
  application.cxx
  LinuxViewerApplication.cxx
  LinuxViewerApplication.h
  LinuxViewerMenuEntries.cxx
  LinuxViewerMenuEntries.h

  LinuxViewerMenuBar.h
)

target_link_libraries(linuxviewer
  PRIVATE
    LinuxViewer::protocols
    LinuxViewer::gtkmmgui
    AICxx::socket-task
    AICxx::resolver-task
    dns::dns
    ${AICXX_OBJECTS_LIST}
    farmhash::farmhash
    blaze::blaze
)

add_subdirectory(GUI_gtkmm3)
add_subdirectory(protocols)
add_subdirectory(data_types)
