#pragma once

#include <vulkan/vulkan.hpp>

namespace vulkan::descriptor {

class SetLayout
{
 private:
   vk::DescriptorSetLayout m_handle;
};

} // namespace vulkan::descriptor
