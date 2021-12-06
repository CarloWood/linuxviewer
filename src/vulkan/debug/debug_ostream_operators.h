#pragma once

#include <vulkan/vulkan.hpp>
#include <type_traits>

#include "Defaults.h"           // For its print_on capability.

// ostream inserters for objects in namespace vk:
#define DECLARE_OSTREAM_INSERTER(vk_class) \
  inline std::ostream& operator<<(std::ostream& os, vk_class const& object) \
  { static_cast<vk_defaults::vk_class const&>(object).print_on(os); return os; }

namespace vk {

// Classes that have vk_defaults.
DECLARE_OSTREAM_INSERTER(Extent2D)
DECLARE_OSTREAM_INSERTER(Extent3D)
DECLARE_OSTREAM_INSERTER(ApplicationInfo)
DECLARE_OSTREAM_INSERTER(InstanceCreateInfo)
DECLARE_OSTREAM_INSERTER(Instance)
DECLARE_OSTREAM_INSERTER(DebugUtilsObjectNameInfoEXT)
DECLARE_OSTREAM_INSERTER(PhysicalDeviceFeatures)
DECLARE_OSTREAM_INSERTER(ImageViewCreateInfo)
DECLARE_OSTREAM_INSERTER(QueueFamilyProperties)
DECLARE_OSTREAM_INSERTER(ExtensionProperties)
DECLARE_OSTREAM_INSERTER(PhysicalDeviceProperties)
DECLARE_OSTREAM_INSERTER(SurfaceCapabilitiesKHR)
DECLARE_OSTREAM_INSERTER(SurfaceFormatKHR)
DECLARE_OSTREAM_INSERTER(SwapchainCreateInfoKHR)
DECLARE_OSTREAM_INSERTER(ImageSubresourceRange)
DECLARE_OSTREAM_INSERTER(GraphicsPipelineCreateInfo)
DECLARE_OSTREAM_INSERTER(PipelineShaderStageCreateInfo)
DECLARE_OSTREAM_INSERTER(PipelineVertexInputStateCreateInfo)
DECLARE_OSTREAM_INSERTER(PipelineInputAssemblyStateCreateInfo)
DECLARE_OSTREAM_INSERTER(PipelineTessellationStateCreateInfo)
DECLARE_OSTREAM_INSERTER(PipelineViewportStateCreateInfo)
DECLARE_OSTREAM_INSERTER(PipelineRasterizationStateCreateInfo)
DECLARE_OSTREAM_INSERTER(PipelineMultisampleStateCreateInfo)
DECLARE_OSTREAM_INSERTER(PipelineDepthStencilStateCreateInfo)
DECLARE_OSTREAM_INSERTER(PipelineColorBlendStateCreateInfo)
DECLARE_OSTREAM_INSERTER(PipelineDynamicStateCreateInfo)
DECLARE_OSTREAM_INSERTER(PipelineColorBlendAttachmentState)
DECLARE_OSTREAM_INSERTER(StencilOpState)
DECLARE_OSTREAM_INSERTER(VertexInputBindingDescription)
DECLARE_OSTREAM_INSERTER(VertexInputAttributeDescription)
DECLARE_OSTREAM_INSERTER(Viewport)
DECLARE_OSTREAM_INSERTER(Rect2D)
DECLARE_OSTREAM_INSERTER(Offset2D)
DECLARE_OSTREAM_INSERTER(SpecializationInfo)
DECLARE_OSTREAM_INSERTER(SpecializationMapEntry)
DECLARE_OSTREAM_INSERTER(ComponentMapping)
DECLARE_OSTREAM_INSERTER(FramebufferCreateInfo)
DECLARE_OSTREAM_INSERTER(MappedMemoryRange)
DECLARE_OSTREAM_INSERTER(SubmitInfo)

template<typename T>
std::ostream& operator<<(std::ostream& os, vk::ArrayProxy<T> const& array_proxy)
{
  os << '<';
  T const* elements = array_proxy.begin();      // vk::ArrayProxy is contiguous.
  for (int i = 0; i < array_proxy.size(); ++i)
  {
    if (i > 0)
      os << ", ";
    os << elements[i];
  }
  return os << '>';
}

} // namespace vk

#undef DECLARE_OSTREAM_INSERTER
