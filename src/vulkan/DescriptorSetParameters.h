#pragma once

namespace vulkan {

// Container class for descriptor related resources.
struct DescriptorSetParameters
{
  vk::UniqueDescriptorPool        m_pool;
  vk::UniqueDescriptorSetLayout   m_layout;
  vk::UniqueDescriptorSet         m_handle;
};

} // namespace vulkan
