#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>

namespace vulkan {

// Struct simplifying render pass creation.
struct RenderPassSubpassData
{
  std::vector<vk::AttachmentReference> const m_input_attachments;
  std::vector<vk::AttachmentReference> const m_color_attachments;
  vk::AttachmentReference const              m_depth_stencil_attachment;
};

} // namespace vulkan
