#pragma once

#include "CommandBuffer.h"
#include "FrameResourcesData.h"
#include "ImageParameters.h"

namespace task {
class SynchronousWindow;
} // namespace task

namespace vulkan {

class ImGui
{
  static constexpr auto pool_type = static_cast<vk::CommandPoolCreateFlags::MaskType>(vulkan::FrameResourcesData::command_pool_type::data_type::create_flags);

  ImageParameters m_font_texture;

 public:
  void init(task::SynchronousWindow const* owning_window, vk::Extent2D extent);

  void on_window_size_changed(vk::Extent2D extent);

  void start_frame(float delta_s);
  void render_frame(CommandBufferWriteAccessType<pool_type>& command_buffer_w);

  ~ImGui();
};

} // namespace vulkan
