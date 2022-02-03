#pragma once

#include "CurrentFrameData.h"
#include "ImageParameters.h"
#include "BufferParameters.h"
#include "DescriptorSetParameters.h"
#include "debug/DebugSetName.h"
#include "debug.h"

#include "CommandBuffer.h"      // CommandBufferWriteAccessType
#include "FrameResourcesData.h" // vulkan::FrameResourcesData::command_pool_type::data_type::create_flags
#include "lvimconfig.h"         // lvImGuiTLS

namespace task {
class SynchronousWindow;
} // namespace task

namespace vulkan {

// Frame resources.
struct ImGui_FrameResourcesData
{
  BufferParameters m_vertex_buffer;
  BufferParameters m_index_buffer;
};

class ImGui
{
  static constexpr auto pool_type = static_cast<vk::CommandPoolCreateFlags::MaskType>(vulkan::FrameResourcesData::command_pool_type::data_type::create_flags);

  task::SynchronousWindow const* m_owning_window;
  ImageParameters m_font_texture;
  utils::Vector<ImGui_FrameResourcesData, FrameResourceIndex> m_frame_resources_list;
  DescriptorSetParameters m_descriptor_set;
  vk::UniquePipelineLayout m_pipeline_layout;
  vk::UniquePipeline m_graphics_pipeline;
  std::filesystem::path m_ini_filename;                 // Cache that io.IniFilename points to.
  ImGuiContext* m_context;                              // The ImGui context of this ImGui instance.

 private:
  inline LogicalDevice const& logical_device() const;

  void setup_render_state(CommandBufferWriteAccessType<pool_type>& command_buffer_w, void* draw_data_void_ptr, ImGui_FrameResourcesData& frame_resources, vk::Viewport const& viewport);
  void create_descriptor_set(
      CWDEBUG_ONLY(AmbifixOwner const& ambifix));
  void create_pipeline_layout(
      CWDEBUG_ONLY(AmbifixOwner const& ambifix));
  void create_graphics_pipeline(
      CWDEBUG_ONLY(AmbifixOwner const& ambifix));

 public:
  void create_frame_resources(FrameResourceIndex number_of_frame_resources
    COMMA_CWDEBUG_ONLY(AmbifixOwner const& ambifix));

  void init(task::SynchronousWindow const* owning_window
      COMMA_CWDEBUG_ONLY(AmbifixOwner const& ambifix));

  // Called at the start of the render loop.
  void set_current_context() { lvImGuiTLS = m_context; }

  void on_window_size_changed(vk::Extent2D extent);
  void on_focus_changed(bool in_focus) const;
  bool on_mouse_move(int x, int y) const;
  void on_mouse_wheel_event(float delta_x, float delta_y) const;
  void on_mouse_click(uint8_t button, bool pressed) const;
  void on_mouse_enter(int x, int y, bool entered) const;
  void on_key_event(uint32_t keysym, bool pressed) const;
  void update_modifiers(int modifiers) const;

  void start_frame(float delta_s);
  void render_frame(CommandBufferWriteAccessType<pool_type>& command_buffer_w, vk::PipelineLayout pipeline_layout, FrameResourceIndex index
      COMMA_CWDEBUG_ONLY(AmbifixOwner const& ambifix));

  ~ImGui();
};

} // namespace vulkan
