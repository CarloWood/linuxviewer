#pragma once

#include <vulkan/vulkan.hpp>

namespace vulkan {

// Struct simplifying render pass creation.
struct RenderPassAttachmentData
{
  vk::Format            m_format;
  vk::AttachmentLoadOp  m_load_op;
  vk::AttachmentStoreOp m_store_op;
  vk::ImageLayout       m_initial_layout;
  vk::ImageLayout       m_final_layout;
};

} // namespace vulkan
