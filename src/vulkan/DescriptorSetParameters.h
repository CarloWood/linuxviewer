#pragma once

#include <vulkan/vulkan.hpp>

namespace vulkan {

// Container class for descriptor related resources.
struct DescriptorSetParameters
{
  vk::UniqueDescriptorPool m_pool;
  vk::UniqueDescriptorSet m_handle;
  vk::UniqueDescriptorSetLayout m_layout;
};

} // namespace vulkan
