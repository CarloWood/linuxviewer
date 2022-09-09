#pragma once

#include "Defaults.h"           // For its print_on capability.
#include <vulkan/vulkan.hpp>
#include <type_traits>
#include <iostream>

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
DECLARE_OSTREAM_INSERTER(QueueFamilyProperties)
DECLARE_OSTREAM_INSERTER(ExtensionProperties)
DECLARE_OSTREAM_INSERTER(PhysicalDeviceProperties)
DECLARE_OSTREAM_INSERTER(PhysicalDeviceMemoryProperties)
DECLARE_OSTREAM_INSERTER(MemoryType)
DECLARE_OSTREAM_INSERTER(MemoryHeap)
DECLARE_OSTREAM_INSERTER(MemoryRequirements)
DECLARE_OSTREAM_INSERTER(BufferCreateInfo)
DECLARE_OSTREAM_INSERTER(ImageCreateInfo)
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
DECLARE_OSTREAM_INSERTER(AttachmentDescription)
DECLARE_OSTREAM_INSERTER(SubpassDescription)

template<typename T>
std::ostream& operator<<(std::ostream& os, ArrayProxy<T> const& array_proxy)
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

template<typename T>
std::ostream& operator<<(std::ostream& os, ArrayWrapper1D<T, 4> const& array_proxy)
{
  os << '<';
  char const* prefix = "";
  for (auto const& element : array_proxy)
  {
    os << prefix << element;
    prefix = ", ";
  }
  return os << '>';
}

std::ostream& operator<<(std::ostream& os, AttachmentReference const& attachment_reference);
std::ostream& operator<<(std::ostream& os, ClearColorValue const& clear_color_value);
std::ostream& operator<<(std::ostream& os, ClearDepthStencilValue const& clear_depth_stencil_value);
std::ostream& operator<<(std::ostream& os, PhysicalDeviceFeatures const& features10);
std::ostream& operator<<(std::ostream& os, PhysicalDeviceFeatures2 const& features2);
std::ostream& operator<<(std::ostream& os, PhysicalDeviceVulkan11Features const& features11);
std::ostream& operator<<(std::ostream& os, PhysicalDeviceVulkan12Features const& features12);
std::ostream& operator<<(std::ostream& os, PipelineCacheCreateInfo const& pipeline_cache_create_info);
std::ostream& operator<<(std::ostream& os, SemaphoreSignalInfo const& semaphores_signal_info);
std::ostream& operator<<(std::ostream& os, SemaphoreWaitInfo const& semaphores_wait_info);
std::ostream& operator<<(std::ostream& os, DescriptorSetLayoutBinding const& descriptor_set_layout_binding);
std::ostream& operator<<(std::ostream& os, DescriptorPoolSize const& descriptor_pool_size);
std::ostream& operator<<(std::ostream& os, DescriptorImageInfo const& descriptor_image_info);
std::ostream& operator<<(std::ostream& os, DescriptorBufferInfo const& descriptor_buffer_info);
std::ostream& operator<<(std::ostream& os, PushConstantRange const& push_constant_range);

} // namespace vk

struct VmaAllocationInfo;
struct VmaAllocationCreateInfo;

std::ostream& operator<<(std::ostream& os, VmaAllocationInfo const& vma_allocation_info);
std::ostream& operator<<(std::ostream& os, VmaAllocationCreateInfo const& vma_allocation_create_info);

#undef DECLARE_OSTREAM_INSERTER
