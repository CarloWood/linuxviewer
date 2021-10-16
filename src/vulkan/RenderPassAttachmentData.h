#pragma once

#include <vulkan/vulkan.hpp>

namespace vulkan {

// Struct simplifying render pass creation.
struct RenderPassAttachmentData
{
  vk::Format            Format;
  vk::AttachmentLoadOp  LoadOp;
  vk::AttachmentStoreOp StoreOp;
  vk::ImageLayout       InitialLayout;
  vk::ImageLayout       FinalLayout;
};

} // namespace vulkan
