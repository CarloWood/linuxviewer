project(practical_vulkan
  LANGUAGES CXX
  DESCRIPTION "Implementation of Frame Resources Count."
)

include(AICxxProject)

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

add_custom_target(frame_resources_count_resources ALL DEPENDS ${RESOURCES_TARGETS})

#==============================================================================

add_executable(frame_resources_count
  FrameResourcesCount.cxx
  FrameResourcesCount.h
  HeavyRectangle.cxx
  HeavyRectangle.h
  RandomPositions.h
  Window.cxx
  Window.h
  LogicalDevice.h
  VertexData.h
  InstanceData.h
  SampleParameters.h
)

target_include_directories(frame_resources_count
  PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(frame_resources_count
  PRIVATE
    LinuxViewer::vulkan
    AICxx::xcb-task
    AICxx::xcb-task::OrgFreedesktopXcbError
    AICxx::resolver-task
    ImGui::imgui
    ${AICXX_OBJECTS_LIST}
    dns::dns
)
