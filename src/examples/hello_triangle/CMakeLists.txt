project(linux_vulkan_engine
  LANGUAGES CXX
  DESCRIPTION "Implementation of Hello Triangle."
)

include(AICxxProject)

#==============================================================================

add_executable(hello_triangle
  HelloTriangle.cxx
  HelloTriangle.h
  Window.cxx
  Window.h
  TrianglePipelineCharacteristic.h
  LogicalDevice.h
)

target_include_directories(hello_triangle
  PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(hello_triangle
  PRIVATE
    LinuxViewer::vulkan
    LinuxViewer::shader_builder
    AICxx::xcb-task
    AICxx::xcb-task::OrgFreedesktopXcbError
    AICxx::resolver-task
    AICxx::block-task
    ImGui::imgui
    ${AICXX_OBJECTS_LIST}
    dns::dns
)
