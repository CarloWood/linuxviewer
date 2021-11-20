#include "sys.h"
#include "Defaults.h"
#include "Application.h"
#include "utils/iomanip.h"

namespace vk_iomanip {

// Used to set and remember a vk::PipelineDynamicStateCreateInfo* on an ostream.
class SetDynamicState : public utils::iomanip::Sticky
{
 private:
  static utils::iomanip::Index s_index;

 public:
  SetDynamicState(vk::PipelineDynamicStateCreateInfo const* pword_value) :
    Sticky(s_index, const_cast<vk::PipelineDynamicStateCreateInfo*>(pword_value)) { }

  static vk::PipelineDynamicStateCreateInfo const* get_pword_value(std::ostream& os)
  {
    return static_cast<vk::PipelineDynamicStateCreateInfo const*>(get_pword_from(os, s_index));
  }
};

//static
utils::iomanip::Index SetDynamicState::s_index;

} // namespace vk_iomanip

namespace vulkan {

//
// Defaults for vulkan::Application
//

int Application::thread_pool_number_of_worker_threads() const
{
  return default_number_of_threads;
}

int Application::thread_pool_queue_capacity(QueuePriority UNUSED_ARG(priority)) const
{
  // By default, make the size of each thread pool queue equal to the number of worker threads.
  return m_thread_pool.number_of_workers();
}

int Application::thread_pool_reserved_threads(QueuePriority UNUSED_ARG(priority)) const
{
  return default_reserved_threads;
}

} // namespace vulkan

#ifdef CWDEBUG
#include "vk_utils/PrintList.h"
#include "vk_utils/PrintPointer.h"
#include "vk_utils/print_version.h"
#include "debug.h"
#include <iomanip>
#endif
#include "vk_utils/print_flags.h"

namespace vk_defaults {

#ifdef CWDEBUG
using vk_utils::print_version;
using vk_utils::print_api_version;
using vk_utils::print_list;
using vk_utils::print_pointer;
using NAMESPACE_DEBUG::print_string;

void ApplicationInfo::print_members(std::ostream& os, char const* prefix) const
{
  os << prefix <<
      "allowDuplicate:"          << std::boolalpha << allowDuplicate <<
    ", pApplicationName:"        << print_string(pApplicationName) <<
    ", applicationVersion:"      << print_version(applicationVersion) <<
    ", pEngineName:"             << print_string(pEngineName) <<
    ", engineVersion:"           << print_version(engineVersion) <<
    ", apiVersion:"              << print_api_version(apiVersion);
}

void InstanceCreateInfo::print_members(std::ostream& os, char const* prefix) const
{
  os << prefix <<
      "flags:"                   << flags <<
    ", pApplicationInfo:"        << (void*)pApplicationInfo;

  if (pApplicationInfo)
    os << " (" << *pApplicationInfo << ')';

  os <<
    ", enabledLayerCount:"       << enabledLayerCount <<
    ", ppEnabledLayerNames:"     << print_list(ppEnabledLayerNames, enabledLayerCount) <<
    ", enabledExtensionCount:"   << enabledExtensionCount <<
    ", ppEnabledExtensionNames:" << print_list(ppEnabledExtensionNames, enabledExtensionCount);
}

void DebugUtilsMessengerCreateInfoEXT::print_members(std::ostream& os, char const* prefix) const
{
  os << prefix;

  if (pNext)
    os << "pNext:" << pNext << ", ";

  os <<
      "flags:"                   << flags <<
    ", messageSeverity:"         << messageSeverity <<
    ", messageType:"             << messageType <<
    ", pfnUserCallback:"         << pfnUserCallback <<
    ", pUserData:"               << pUserData;
}

void DebugUtilsObjectNameInfoEXT::print_members(std::ostream& os, char const* prefix) const
{
  os << prefix;

  if (pNext)
    os << "pNext:" << pNext << ", ";

  using std::hex;
  os <<
      "objectType:"              << to_string(objectType) <<
    ", objectHandle:"     << hex << objectHandle <<
    ", pObjectName:"             << print_string(pObjectName);
}

void PhysicalDeviceFeatures::print_members(std::ostream& os, char const* prefix) const
{
  os << prefix;

  // All `var` are VkBool32.
#define SHOW_IF_TRUE(var) \
  do { if (var) { os << prefix << #var; prefix = ", "; } } while(0)

  prefix = "<";
  SHOW_IF_TRUE(robustBufferAccess);
  SHOW_IF_TRUE(fullDrawIndexUint32);
  SHOW_IF_TRUE(imageCubeArray);
  SHOW_IF_TRUE(independentBlend);
  SHOW_IF_TRUE(geometryShader);
  SHOW_IF_TRUE(tessellationShader);
  SHOW_IF_TRUE(sampleRateShading);
  SHOW_IF_TRUE(dualSrcBlend);
  SHOW_IF_TRUE(logicOp);
  SHOW_IF_TRUE(multiDrawIndirect);
  SHOW_IF_TRUE(drawIndirectFirstInstance);
  SHOW_IF_TRUE(depthClamp);
  SHOW_IF_TRUE(depthBiasClamp);
  SHOW_IF_TRUE(fillModeNonSolid);
  SHOW_IF_TRUE(depthBounds);
  SHOW_IF_TRUE(wideLines);
  SHOW_IF_TRUE(largePoints);
  SHOW_IF_TRUE(alphaToOne);
  SHOW_IF_TRUE(multiViewport);
  SHOW_IF_TRUE(samplerAnisotropy);
  SHOW_IF_TRUE(textureCompressionETC2);
  SHOW_IF_TRUE(textureCompressionASTC_LDR);
  SHOW_IF_TRUE(textureCompressionBC);
  SHOW_IF_TRUE(occlusionQueryPrecise);
  SHOW_IF_TRUE(pipelineStatisticsQuery);
  SHOW_IF_TRUE(vertexPipelineStoresAndAtomics);
  SHOW_IF_TRUE(fragmentStoresAndAtomics);
  SHOW_IF_TRUE(shaderTessellationAndGeometryPointSize);
  SHOW_IF_TRUE(shaderImageGatherExtended);
  SHOW_IF_TRUE(shaderStorageImageExtendedFormats);
  SHOW_IF_TRUE(shaderStorageImageMultisample);
  SHOW_IF_TRUE(shaderStorageImageReadWithoutFormat);
  SHOW_IF_TRUE(shaderStorageImageWriteWithoutFormat);
  SHOW_IF_TRUE(shaderUniformBufferArrayDynamicIndexing);
  SHOW_IF_TRUE(shaderSampledImageArrayDynamicIndexing);
  SHOW_IF_TRUE(shaderStorageBufferArrayDynamicIndexing);
  SHOW_IF_TRUE(shaderStorageImageArrayDynamicIndexing);
  SHOW_IF_TRUE(shaderClipDistance);
  SHOW_IF_TRUE(shaderCullDistance);
  SHOW_IF_TRUE(shaderFloat64);
  SHOW_IF_TRUE(shaderInt64);
  SHOW_IF_TRUE(shaderInt16);
  SHOW_IF_TRUE(shaderResourceResidency);
  SHOW_IF_TRUE(shaderResourceMinLod);
  SHOW_IF_TRUE(sparseBinding);
  SHOW_IF_TRUE(sparseResidencyBuffer);
  SHOW_IF_TRUE(sparseResidencyImage2D);
  SHOW_IF_TRUE(sparseResidencyImage3D);
  SHOW_IF_TRUE(sparseResidency2Samples);
  SHOW_IF_TRUE(sparseResidency4Samples);
  SHOW_IF_TRUE(sparseResidency8Samples);
  SHOW_IF_TRUE(sparseResidency16Samples);
  SHOW_IF_TRUE(sparseResidencyAliased);
  SHOW_IF_TRUE(variableMultisampleRate);
  SHOW_IF_TRUE(inheritedQueries);
  os << ">";

#undef SHOW_IF_TRUE
}

#endif // CWDEBUG

void DeviceCreateInfo::print_members(std::ostream& os, char const* prefix) const
{
  os << prefix;

  if (pNext)
    os << "pNext:" << pNext << ", ";

  os << "flags:" << flags <<
      ", pQueueCreateInfos:<";
  for (int i = 0; i < queueCreateInfoCount; ++i)
  {
    if (i > 0)
      os << ',';
    static_cast<DeviceQueueCreateInfo const&>(pQueueCreateInfos[i]).print_members(os, "");
  }
  os << ">, ppEnabledLayerNames:<";
  for (int i = 0; i < enabledLayerCount; ++i)
  {
    if (i > 0)
      os << ',';
    os << '"' << ppEnabledLayerNames[i] << '"';
  }
  os << ">, ppEnabledExtensionNames:<";
  for (int i = 0; i < enabledExtensionCount; ++i)
  {
    if (i > 0)
      os << ',';
    os << '"' << ppEnabledExtensionNames[i] << '"';
  }
  os << ">";
#ifdef CWDEBUG
  os << ", pEnabledFeatures";
  if (pEnabledFeatures)
    os << "->" << *pEnabledFeatures;
  else
    os << ":nullptr";
#endif
}

void DeviceQueueCreateInfo::print_members(std::ostream& os, char const* prefix) const
{
  os << prefix;

  if (pNext)
    os << "pNext:" << pNext << ", ";
  os << "queueFamilyIndex:" << queueFamilyIndex <<
//      ", queueCount: " << queueCount <<
      ", pQueuePriorities:" << print_list(pQueuePriorities, queueCount);
}

#ifdef CWDEBUG
void ImageCreateInfo::print_members(std::ostream& os, char const* prefix) const
{
  os << prefix << "flags:" << flags <<
      ", imageType:" << imageType <<
      ", format:" << format <<
      ", extent:" << extent <<
      ", mipLevels:" << mipLevels <<
      ", arrayLayers:" << arrayLayers <<
      ", samples:" << samples <<
      ", tiling:" << tiling <<
      ", usage:" << usage <<
      ", sharingMode:" << sharingMode;
    if (sharingMode == vk::SharingMode::eConcurrent)
    {
//    os << ", queueFamilyIndexCount:" << queueFamilyIndexCount;
      os << ", pQueueFamilyIndices:" << print_list(pQueueFamilyIndices, queueFamilyIndexCount);
    }
  os << ", initialLayout:" << initialLayout;
}

void ImageViewCreateInfo::print_members(std::ostream& os, char const* prefix) const
{
  os << prefix << "flags:" << flags <<
      ", image:" << image <<
      ", viewType:" << viewType <<
      ", format:" << format <<
      ", components:" << components <<
      ", subresourceRange:" << subresourceRange;
}

void Extent2D::print_members(std::ostream& os, char const* prefix) const
{
  os << prefix << "width:" << width <<
      ", height:" << height;
}

void Extent3D::print_members(std::ostream& os, char const* prefix) const
{
  os << prefix << "width:" << width <<
      ", height:" << height <<
      ", depth:" << depth;
}

void Instance::print_members(std::ostream& os, char const* prefix) const
{
  os << prefix << "m_instance: " << this->operator VkInstance();
}

void QueueFamilyProperties::print_members(std::ostream& os, char const* prefix) const
{
  os << prefix << "queueFlags:" << queueFlags <<
      ", queueCount:" << queueCount <<
      ", timestampValidBits:" << timestampValidBits <<
      ", minImageTransferGranularity:" << minImageTransferGranularity;
}

void ExtensionProperties::print_members(std::ostream& os, char const* prefix) const
{
  os << prefix << "extensionName:" << print_string(extensionName) <<
      ", specVersion:" << specVersion;
}

void PhysicalDeviceProperties::print_members(std::ostream& os, char const* prefix) const
{
  os << prefix << "apiVersion:" << print_api_version(apiVersion) <<
      ", driverVersion:" << print_api_version(driverVersion) <<
      ", vendorID:0x" << std::hex << vendorID <<
      ", deviceID:0x" << std::hex << deviceID <<
      ", deviceType:" << to_string(deviceType) <<
      ", deviceName:" << print_string(deviceName);
//  os << "pipelineCacheUUID:" << UUID(reinterpret_cast<char const*>(static_cast<uint8_t const*>(physical_device_properties.pipelineCacheUUID))) << ", ";
//    VULKAN_HPP_NAMESPACE::PhysicalDeviceLimits                                   limits            = {};
//    VULKAN_HPP_NAMESPACE::PhysicalDeviceSparseProperties                         sparseProperties  = {};
}

void SurfaceCapabilitiesKHR::print_members(std::ostream& os, char const* prefix) const
{
  os << prefix << "minImageCount:" << minImageCount <<
      ", maxImageCount:" << maxImageCount <<
      ", currentExtent:" << currentExtent <<
      ", minImageExtent:" << minImageExtent <<
      ", maxImageExtent:" << maxImageExtent <<
      ", maxImageArrayLayers:" << maxImageArrayLayers <<
      ", supportedTransforms:" << supportedTransforms <<
      ", currentTransform:" << currentTransform <<
      ", supportedCompositeAlpha:" << supportedCompositeAlpha <<
      ", supportedUsageFlags:" << supportedUsageFlags;
}

void SurfaceFormatKHR::print_members(std::ostream& os, char const* prefix) const
{
  os << prefix << "format:" << format <<
      ", colorSpace:" << colorSpace;
}

void SwapchainCreateInfoKHR::print_members(std::ostream& os, char const* prefix) const
{
  os << prefix;

  if (pNext)
    os << "pNext:" << pNext << ", ";

  os << "flags:" << flags <<
      ", surface:" << surface <<
      ", minImageCount:" << minImageCount <<
      ", imageFormat:" << imageFormat <<
      ", imageColorSpace:" << imageColorSpace <<
      ", imageExtent:" << imageExtent <<
      ", imageArrayLayers:" << imageArrayLayers <<
      ", imageUsage:" << imageUsage <<
      ", imageSharingMode:" << imageSharingMode <<
//      ", queueFamilyIndexCount:" << queueFamilyIndexCount <<
      ", pQueueFamilyIndices:" << print_list(pQueueFamilyIndices, queueFamilyIndexCount) <<
      ", preTransform:" << preTransform <<
      ", compositeAlpha:" << compositeAlpha <<
      ", presentMode:" << presentMode <<
      ", clipped:" << clipped <<
      ", oldSwapchain:" << oldSwapchain;
}

void ImageSubresourceRange::print_members(std::ostream& os, char const* prefix) const
{
  os << prefix << "aspectMask:" << aspectMask <<
      ", baseMipLevel:" << baseMipLevel <<
      ", levelCount:" << levelCount <<
      ", baseArrayLayer:" << baseArrayLayer <<
      ", layerCount:" << layerCount;
}

void GraphicsPipelineCreateInfo::print_members(std::ostream& os, char const* prefix) const
{
  os << prefix;

  if (pNext)
    os << "pNext:" << pNext << ", ";

  os << "flags:" << flags <<
      ", stageCount:" << stageCount <<
      ", pStages:" << print_list(pStages, stageCount) <<
      vk_iomanip::SetDynamicState(pDynamicState) <<                     // Need to remember the dynamic state in order to print these members correctly.
      ", pVertexInputState:" << print_pointer(pVertexInputState) <<
      ", pInputAssemblyState:" << print_pointer(pInputAssemblyState) <<
      ", pTessellationState:" << print_pointer(pTessellationState) <<
      ", pViewportState:" << print_pointer(pViewportState) <<
      ", pRasterizationState:" << print_pointer(pRasterizationState) <<
      ", pMultisampleState:" << print_pointer(pMultisampleState) <<
      ", pDepthStencilState:" << print_pointer(pDepthStencilState) <<
      ", pColorBlendState:" << print_pointer(pColorBlendState) <<
      ", pDynamicState:" << print_pointer(pDynamicState) <<
      ", layout:" << layout <<
      ", renderPass:" << renderPass <<
      ", subpass:" << subpass <<
      ", basePipelineHandle:" << basePipelineHandle <<
      ", basePipelineIndex:" << basePipelineIndex;
}

void PipelineShaderStageCreateInfo::print_members(std::ostream& os, char const* prefix) const
{
  os << prefix;

  if (pNext)
    os << "pNext:" << pNext << ", ";

  os << "flags:" << flags <<
      ", stage:" << stage <<
      ", module:" << module <<
      ", pName:" << print_string(pName) <<
      ", pSpecializationInfo:" << print_pointer(pSpecializationInfo);
}

void PipelineVertexInputStateCreateInfo::print_members(std::ostream& os, char const* prefix) const
{
  os << prefix;

  if (pNext)
    os << "pNext:" << pNext << ", ";

  os << "flags:" << flags <<
//      ", vertexBindingDescriptionCount:" << vertexBindingDescriptionCount <<
      ", pVertexBindingDescriptions:" << print_list(pVertexBindingDescriptions, vertexBindingDescriptionCount) <<
//      ", vertexAttributeDescriptionCount:" << vertexAttributeDescriptionCount <<
      ", pVertexAttributeDescriptions:" << print_list(pVertexAttributeDescriptions, vertexAttributeDescriptionCount);
}

void PipelineInputAssemblyStateCreateInfo::print_members(std::ostream& os, char const* prefix) const
{
  os << prefix;

  if (pNext)
    os << "pNext:" << pNext << ", ";

  os << "flags:" << flags <<
      ", topology:" << topology <<
      ", primitiveRestartEnable:" << primitiveRestartEnable;
}

void PipelineTessellationStateCreateInfo::print_members(std::ostream& os, char const* prefix) const
{
  os << prefix;

  if (pNext)
    os << "pNext:" << pNext << ", ";

  os << "flags:" << flags <<
      ", patchControlPoints:" << patchControlPoints;
}

void PipelineViewportStateCreateInfo::print_members(std::ostream& os, char const* prefix) const
{
  vk::PipelineDynamicStateCreateInfo const* pipeline_dynamic_state_create_info = vk_iomanip::SetDynamicState::get_pword_value(os);
  // You must use vk_iomanip::SetDynamicState(pDynamicState) before printing a PipelineViewportStateCreateInfo object.
  ASSERT(pipeline_dynamic_state_create_info);
  bool has_dynamic_viewports = false;
  bool has_dynamic_scissors = false;
  for (int i = 0; i < pipeline_dynamic_state_create_info->dynamicStateCount; ++i)
  {
    if (pipeline_dynamic_state_create_info->pDynamicStates[i] == vk::DynamicState::eViewport)
      has_dynamic_viewports = true;
    if (pipeline_dynamic_state_create_info->pDynamicStates[i] == vk::DynamicState::eScissor)
      has_dynamic_scissors = true;
  }

  os << prefix;

  if (pNext)
    os << "pNext:" << pNext << ", ";

  os << "flags:" << flags;

  if (has_dynamic_viewports)
    os << ", viewportCount:" << viewportCount;
  else
    os << ", pViewports:" << print_list(pViewports, viewportCount);

  if (has_dynamic_scissors)
    os << ", scissorCount:" << scissorCount;
  else
    os << ", pScissors:" << print_list(pScissors, scissorCount);
}

void PipelineRasterizationStateCreateInfo::print_members(std::ostream& os, char const* prefix) const
{
  os << prefix;

  if (pNext)
    os << "pNext:" << pNext << ", ";

  os << "flags:" << flags <<
      ", depthClampEnable:" << depthClampEnable <<
      ", rasterizerDiscardEnable:" << rasterizerDiscardEnable <<
      ", polygonMode:" << polygonMode <<
      ", cullMode:" << cullMode <<
      ", frontFace:" << frontFace <<
      ", depthBiasEnable:" << depthBiasEnable <<
      ", depthBiasConstantFactor:" << depthBiasConstantFactor <<
      ", depthBiasClamp:" << depthBiasClamp <<
      ", depthBiasSlopeFactor:" << depthBiasSlopeFactor <<
      ", lineWidth:" << lineWidth;
}

void PipelineMultisampleStateCreateInfo::print_members(std::ostream& os, char const* prefix) const
{
  os << prefix;

  if (pNext)
    os << "pNext:" << pNext << ", ";

  os << "flags:" << flags <<
      ", rasterizationSamples:" << rasterizationSamples <<
      ", sampleShadingEnable:" << sampleShadingEnable <<
      ", minSampleShading:" << minSampleShading <<
      ", pSampleMask:" << print_pointer(pSampleMask) <<
      ", alphaToCoverageEnable:" << alphaToCoverageEnable <<
      ", alphaToOneEnable:" << alphaToOneEnable;
}

void PipelineDepthStencilStateCreateInfo::print_members(std::ostream& os, char const* prefix) const
{
  os << prefix;

  if (pNext)
    os << "pNext:" << pNext << ", ";

  os << "flags:" << flags <<
      ", depthTestEnable:" << depthTestEnable <<
      ", depthWriteEnable:" << depthWriteEnable <<
      ", depthCompareOp:" << depthCompareOp <<
      ", depthBoundsTestEnable:" << depthBoundsTestEnable <<
      ", stencilTestEnable:" << stencilTestEnable <<
      ", front:" << front <<
      ", back:" << back <<
      ", minDepthBounds:" << minDepthBounds <<
      ", maxDepthBounds:" << maxDepthBounds;
}

void PipelineColorBlendStateCreateInfo::print_members(std::ostream& os, char const* prefix) const
{
  os << prefix;

  if (pNext)
    os << "pNext:" << pNext << ", ";

  os << "flags:" << flags <<
      ", logicOpEnable:" << logicOpEnable <<
      ", logicOp:" << logicOp <<
//      ", attachmentCount:" << attachmentCount <<
      ", pAttachments:" << print_list(pAttachments, attachmentCount) <<
      ", blendConstants:" << blendConstants;
}

void PipelineDynamicStateCreateInfo::print_members(std::ostream& os, char const* prefix) const
{
  os << prefix;

  if (pNext)
    os << "pNext:" << pNext << ", ";

  os << "flags:" << flags <<
//      ", dynamicStateCount:" << dynamicStateCount <<
      ", pDynamicStates:" << print_list(pDynamicStates, dynamicStateCount);
}

void PipelineColorBlendAttachmentState::print_members(std::ostream& os, char const* prefix) const
{
  os << "blendEnable:" << blendEnable <<
      ", srcColorBlendFactor:" << srcColorBlendFactor <<
      ", dstColorBlendFactor:" << dstColorBlendFactor <<
      ", colorBlendOp:" << colorBlendOp <<
      ", srcAlphaBlendFactor:" << srcAlphaBlendFactor <<
      ", dstAlphaBlendFactor:" << dstAlphaBlendFactor <<
      ", alphaBlendOp:" << alphaBlendOp <<
      ", colorWriteMask:" << colorWriteMask;
}

void StencilOpState::print_members(std::ostream& os, char const* prefix) const
{
  os << "failOp:" << failOp <<
      ", passOp:" << passOp <<
      ", depthFailOp:" << depthFailOp <<
      ", compareOp:" << compareOp <<
      ", compareMask:" << compareMask <<
      ", writeMask:" << writeMask <<
      ", reference:" << reference;
}

void VertexInputBindingDescription::print_members(std::ostream& os, char const* prefix) const
{
  os << "binding:" << binding <<
      ", stride:" << stride <<
      ", inputRate:" << inputRate;
}

void VertexInputAttributeDescription::print_members(std::ostream& os, char const* prefix) const
{
  os << "location:" << location <<
      ", binding:" << binding <<
      ", format:" << format <<
      ", offset:" << offset;
}

void Viewport::print_members(std::ostream& os, char const* prefix) const
{
  os << "x:" << x <<
      ", y:" << y <<
      ", width:" << width <<
      ", height:" << height <<
      ", minDepth:" << minDepth <<
      ", maxDepth:" << maxDepth;
}

void Rect2D::print_members(std::ostream& os, char const* prefix) const
{
  os << "offset:" << offset <<
      ", extent:" << extent;
}

void Offset2D::print_members(std::ostream& os, char const* prefix) const
{
  os << "x:" << x <<
      ", y:" << y;
}

void SpecializationInfo::print_members(std::ostream& os, char const* prefix) const
{
  os <<
//        "mapEntryCount:" << mapEntryCount << ", "
        "pMapEntries:" << print_list(pMapEntries, mapEntryCount) <<
     ", dataSize:" << dataSize <<
      ", pData:" << pData;
}

void SpecializationMapEntry::print_members(std::ostream& os, char const* prefix) const
{
  os << "constantID:" << constantID <<
      ", offset:" << offset <<
      ", size:" << size;
}

void ComponentMapping::print_members(std::ostream& os, char const* prefix) const
{
  os << "r:" << r <<
      ", g:" << g <<
      ", b:" << b <<
      ", a:" << a;
}

void FramebufferCreateInfo::print_members(std::ostream& os, char const* prefix) const
{
  os << prefix;

  if (pNext)
    os << "pNext:" << pNext << ", ";

  os << "flags:" << flags <<
      ", renderPass:" << renderPass;
  if ((flags & vk::FramebufferCreateFlagBits::eImageless))
    os << ", attachmentCount:" << attachmentCount;
  else
    os << ", pAttachments:" << print_list(pAttachments, attachmentCount);
  os << ", width:" << width <<
      ", height:" << height <<
      ", layers:" << layers;
}

void MappedMemoryRange::print_members(std::ostream& os, char const* prefix) const
{
  os << prefix;

  if (pNext)
    os << "pNext:" << pNext << ", ";

  os << "memory:" << memory <<
      ", offset:" << offset <<
      ", size:" << size;
}

#endif // CWDEBUG

} // namespace vk_defaults

#ifdef CWDEBUG

#if !defined(DOXYGEN)
NAMESPACE_DEBUG_CHANNELS_START
channel_ct vulkan("VULKAN");
channel_ct vkframe("VKFRAME");
channel_ct vkverbose("VKVERBOSE");
channel_ct vkinfo("VKINFO");
channel_ct vkwarning("VKWARNING");
channel_ct vkerror("VKERROR");
NAMESPACE_DEBUG_CHANNELS_END
#endif

namespace vk_defaults {

void debug_init()
{
  if (!DEBUGCHANNELS::dc::vkerror.is_on())
  {
    DoutFatal(dc::core, "vkerror must be turned on from .libcwdrc (and so do all other vk* debug channels).");
    DEBUGCHANNELS::dc::vkerror.on();
  }
  if (!DEBUGCHANNELS::dc::vkwarning.is_on() && DEBUGCHANNELS::dc::warning.is_on())
    DEBUGCHANNELS::dc::vkwarning.on();
}

} // namespace vk_defaults

#endif // CWDEBUG
