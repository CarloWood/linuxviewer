#pragma once

#include <vulkan/vulkan.hpp>

namespace vulkan {

// Container class for descriptor related resources.
struct DescriptorSetParameters
{
  vk::UniqueDescriptorSetLayout m_layout;
  vk::UniqueDescriptorPool m_pool;
  vk::UniqueDescriptorSet m_handle;
};

} // namespace vulkan
