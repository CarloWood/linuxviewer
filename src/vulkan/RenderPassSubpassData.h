#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>

namespace vulkan {

// Struct simplifying render pass creation.
struct RenderPassSubpassData
{
  std::vector<vk::AttachmentReference> const  InputAttachments;
  std::vector<vk::AttachmentReference> const  ColorAttachments;
  vk::AttachmentReference const               DepthStencilAttachment;
};

} // namespace vulkan
