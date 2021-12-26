#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>

namespace vulkan {

// Struct simplifying render pass creation.
struct RenderPassSubpassData
{
  std::vector<vk::AttachmentReference> m_input_attachments;
  std::vector<vk::AttachmentReference> m_color_attachments;
  vk::AttachmentReference              m_depth_stencil_attachment;
};

} // namespace vulkan
