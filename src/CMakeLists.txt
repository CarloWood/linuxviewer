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

#==============================================================================
# RESOURCES
#

set(RESOURCES_SUBDIRS textures)

set(RESOURCES_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/resources)
set(RESOURCES_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/data)

set(resources_src_files "")
set(resources_dst_files "")
foreach(subdir IN LISTS RESOURCES_SUBDIRS)
  file(GLOB_RECURSE resource_src_files ${RESOURCES_SOURCE_DIR}/${subdir}/*)
  list(APPEND resources_src_files ${resource_src_files})
  foreach(file IN LISTS resource_src_files)
    file(RELATIVE_PATH resource_dst_file ${RESOURCES_SOURCE_DIR} ${file})
    list(APPEND resources_dst_files ${RESOURCES_BINARY_DIR}/${resource_dst_file})
  endforeach()
endforeach()

foreach(subdir IN LISTS RESOURCES_SUBDIRS)
  add_custom_command(
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${RESOURCES_SOURCE_DIR}/${subdir} ${RESOURCES_BINARY_DIR}/${subdir}
    OUTPUT  ${resources_dst_files}
    DEPENDS ${resources_src_files}
    COMMENT "Copying ${subdir}"
  )
  list(APPEND RESOURCES_TARGETS ${resources_dst_files})
endforeach()

add_custom_target(resources ALL DEPENDS ${RESOURCES_TARGETS})

#==============================================================================

add_executable(mytest
  TestApplication.cxx
  TestApplication.h
  TimerData.cxx
  TimerData.h
)

target_link_libraries(mytest
  PRIVATE
    LinuxViewer::vulkan
    AICxx::xcb-task
    AICxx::xcb-task::OrgFreedesktopXcbError
    AICxx::resolver-task
    dns::dns
    ${AICXX_OBJECTS_LIST}
    ImGui::imgui
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
    AICxx::socket-task
    AICxx::resolver-task
    AICxx::xmlrpc-task
    LinuxViewer::vulkan
    AICxx::xcb-task
    AICxx::xcb-task::OrgFreedesktopXcbError
    dns::dns
    ${AICXX_OBJECTS_LIST}
    farmhash::farmhash
    blaze::blaze
    Boost::serialization
)

add_subdirectory(protocols)
add_subdirectory(data_types)

# Vulkan stuff.
add_subdirectory(vulkan)
