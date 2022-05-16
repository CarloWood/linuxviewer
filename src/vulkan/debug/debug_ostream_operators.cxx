#include "sys.h"
#include "rendergraph/AttachmentNode.h"
#include "debug_ostream_operators.h"
#include "vk_utils/print_version.h"
#include "vk_utils/print_chain.h"
#include "vk_utils/print_flags.h"
#include "vk_utils/print_list.h"
#include <iostream>

namespace vk {

using NAMESPACE_DEBUG::print_string;
using vk_utils::print_api_version;
using vk_utils::print_list;

std::ostream& operator<<(std::ostream& os, vk::AttachmentReference const& attachment_reference)
{
  os << '{';
  vulkan::rendergraph::pAttachmentsIndex attachment{attachment_reference.attachment};
  os << "attachment:";
  if (attachment_reference.attachment == VK_ATTACHMENT_UNUSED)
    os << "VK_ATTACHMENT_UNUSED";
  else
    os << attachment;
  os << ", layout:" << to_string(attachment_reference.layout);
  os << '}';
  return os;
}

#ifdef CWDEBUG
std::ostream& operator<<(std::ostream& os, vk::ClearColorValue const& value)
{
  // Try to heuristically determine the format.
  // It is not possible to distinguish all zeroes, so lets call that just "zeroed".
  bool is_zeroed = true;
  bool is_float = true;
  bool is_unsigned = true;
  bool is_signed = true;
  bool is_super_small_float = true;
  for (int i = 0; i < 4; ++i)
  {
    if (value.uint32[i] == 0)
      continue;
    is_zeroed = false;
    if (value.uint32[i] > 1000000)              // Probably just negative uint.
      is_unsigned = false;
    if (std::isnan(value.float32[i]) || value.float32[i] < -1.f || value.float32[i] > 100.f) // Should really be between 0 and 1, but well.
      is_float = false;
    else if (value.float32[i] > 10e-40)
      is_super_small_float = false;
    if (value.int32[i] < -100000 || value.int32[i] > 1000000)
      is_signed = false;
  }
  if (is_super_small_float && !is_zeroed)
    is_float = false;
  os << '{';
  if (is_zeroed)
    os << "<zeroed>";
  else
  {
    if (is_float)
    {
      os << "float32:" << value.float32;
      if (is_unsigned || is_signed)
        os << " / ";
    }
    if (is_unsigned)
    {
      if (!is_signed)
        os << "uint32:";
      os << value.uint32;
    }
    else if (is_signed)
      os << "int32:" << value.int32;
  }
  os << '}';
  return os;
}

std::ostream& operator<<(std::ostream& os, vk::ClearDepthStencilValue const& value)
{
  os << '{';
  os << "depth:" << value.depth <<
      ", stencil:0x" << std::hex << value.stencil << std::dec;
  os << '}';
  return os;
}

std::ostream& operator<<(std::ostream& os, PhysicalDeviceFeatures2 const& features2)
{
  os << '{';
  os << "features:" << features2.features;
  if (features2.pNext)
    os << vk_utils::print_chain(features2.pNext);
  os << '}';
  return os;
}

std::ostream& operator<<(std::ostream& os, PhysicalDeviceFeatures const& features10)
{
  os << '{';

  // All `var` are VkBool32.
#define SHOW_IF_TRUE(var) \
  do { if (features10.var) { os << prefix << #var; prefix = ", "; } } while(0)

  os << '<';
  char const* prefix = "";
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
  os << '>';

  os << '}';
  return os;

#undef SHOW_IF_TRUE
}

std::ostream& operator<<(std::ostream& os, PhysicalDeviceVulkan11Features const& features11)
{
  bool const in_chain = vk_utils::ChainManipulator::get_iword_value(os);

  if (!in_chain)
    os << '{';

  // All `var` are VkBool32.
#define SHOW_IF_TRUE(var) \
  do { if (features11.var) { os << prefix << #var; prefix = ", "; } } while(0)

  os << '<';
  char const* prefix = "";
  SHOW_IF_TRUE(storageBuffer16BitAccess);
  SHOW_IF_TRUE(uniformAndStorageBuffer16BitAccess);
  SHOW_IF_TRUE(storagePushConstant16);
  SHOW_IF_TRUE(storageInputOutput16);
  SHOW_IF_TRUE(multiview);
  SHOW_IF_TRUE(multiviewGeometryShader);
  SHOW_IF_TRUE(multiviewTessellationShader);
  SHOW_IF_TRUE(variablePointersStorageBuffer);
  SHOW_IF_TRUE(variablePointers);
  SHOW_IF_TRUE(protectedMemory);
  SHOW_IF_TRUE(samplerYcbcrConversion);
  SHOW_IF_TRUE(shaderDrawParameters);
  os << '>';

  if (!in_chain)
  {
    if (features11.pNext)
      os << vk_utils::print_chain(features11.pNext);
    os << '}';
  }

#undef SHOW_IF_TRUE
  return os;
}

std::ostream& operator<<(std::ostream& os, PhysicalDeviceVulkan12Features const& features12)
{
  bool const in_chain = vk_utils::ChainManipulator::get_iword_value(os);

  if (!in_chain)
    os << '{';

  // All `var` are VkBool32.
#define SHOW_IF_TRUE(var) \
  do { if (features12.var) { os << prefix << #var; prefix = ", "; } } while(0)

  os << '<';
  char const* prefix = "";
  SHOW_IF_TRUE(samplerMirrorClampToEdge);
  SHOW_IF_TRUE(drawIndirectCount);
  SHOW_IF_TRUE(storageBuffer8BitAccess);
  SHOW_IF_TRUE(uniformAndStorageBuffer8BitAccess);
  SHOW_IF_TRUE(storagePushConstant8);
  SHOW_IF_TRUE(shaderBufferInt64Atomics);
  SHOW_IF_TRUE(shaderSharedInt64Atomics);
  SHOW_IF_TRUE(shaderFloat16);
  SHOW_IF_TRUE(shaderInt8);
  SHOW_IF_TRUE(descriptorIndexing);
  SHOW_IF_TRUE(shaderInputAttachmentArrayDynamicIndexing);
  SHOW_IF_TRUE(shaderUniformTexelBufferArrayDynamicIndexing);
  SHOW_IF_TRUE(shaderStorageTexelBufferArrayDynamicIndexing);
  SHOW_IF_TRUE(shaderUniformBufferArrayNonUniformIndexing);
  SHOW_IF_TRUE(shaderSampledImageArrayNonUniformIndexing);
  SHOW_IF_TRUE(shaderStorageBufferArrayNonUniformIndexing);
  SHOW_IF_TRUE(shaderStorageImageArrayNonUniformIndexing);
  SHOW_IF_TRUE(shaderInputAttachmentArrayNonUniformIndexing);
  SHOW_IF_TRUE(shaderUniformTexelBufferArrayNonUniformIndexing);
  SHOW_IF_TRUE(shaderStorageTexelBufferArrayNonUniformIndexing);
  SHOW_IF_TRUE(descriptorBindingUniformBufferUpdateAfterBind);
  SHOW_IF_TRUE(descriptorBindingSampledImageUpdateAfterBind);
  SHOW_IF_TRUE(descriptorBindingStorageImageUpdateAfterBind);
  SHOW_IF_TRUE(descriptorBindingStorageBufferUpdateAfterBind);
  SHOW_IF_TRUE(descriptorBindingUniformTexelBufferUpdateAfterBind);
  SHOW_IF_TRUE(descriptorBindingStorageTexelBufferUpdateAfterBind);
  SHOW_IF_TRUE(descriptorBindingUpdateUnusedWhilePending);
  SHOW_IF_TRUE(descriptorBindingPartiallyBound);
  SHOW_IF_TRUE(descriptorBindingVariableDescriptorCount);
  SHOW_IF_TRUE(runtimeDescriptorArray);
  SHOW_IF_TRUE(samplerFilterMinmax);
  SHOW_IF_TRUE(scalarBlockLayout);
  SHOW_IF_TRUE(imagelessFramebuffer);
  SHOW_IF_TRUE(uniformBufferStandardLayout);
  SHOW_IF_TRUE(shaderSubgroupExtendedTypes);
  SHOW_IF_TRUE(separateDepthStencilLayouts);
  SHOW_IF_TRUE(hostQueryReset);
  SHOW_IF_TRUE(timelineSemaphore);
  SHOW_IF_TRUE(bufferDeviceAddress);
  SHOW_IF_TRUE(bufferDeviceAddressCaptureReplay);
  SHOW_IF_TRUE(bufferDeviceAddressMultiDevice);
  SHOW_IF_TRUE(vulkanMemoryModel);
  SHOW_IF_TRUE(vulkanMemoryModelDeviceScope);
  SHOW_IF_TRUE(vulkanMemoryModelAvailabilityVisibilityChains);
  SHOW_IF_TRUE(shaderOutputViewportIndex);
  SHOW_IF_TRUE(shaderOutputLayer);
  SHOW_IF_TRUE(subgroupBroadcastDynamicId);
  os << '>';

  if (!in_chain)
  {
    if (features12.pNext)
      os << vk_utils::print_chain(features12.pNext);
    os << '}';
  }

#undef SHOW_IF_TRUE
  return os;
}

std::ostream& operator<<(std::ostream& os, PipelineCacheCreateInfo const& pipeline_cache_create_info)
{
  os << '{';
  if (pipeline_cache_create_info.pNext)
    os << "pNext:" << pipeline_cache_create_info.pNext << ", ";
  os << "flags:" << pipeline_cache_create_info.flags <<
      ", initialDataSize:" << pipeline_cache_create_info.initialDataSize <<
      ", pInitialData:" << (void*)pipeline_cache_create_info.pInitialData;
  os << '}';
  return os;
}

std::ostream& operator<<(std::ostream& os, SemaphoreSignalInfo const& semaphores_signal_info)
{
  os << '{';
  if (semaphores_signal_info.pNext)
    os << "pNext:" << semaphores_signal_info.pNext << ", ";
  os << "semaphore:" << semaphores_signal_info.semaphore <<
      ", value:" << semaphores_signal_info.value;
  os << '}';
  return os;
}

std::ostream& operator<<(std::ostream& os, SemaphoreWaitInfo const& semaphores_wait_info)
{
  os << '{';
  if (semaphores_wait_info.pNext)
    os << "pNext:" << semaphores_wait_info.pNext << ", ";
  os << "flags:" << semaphores_wait_info.flags <<
      ", pSemaphores:" << print_list(semaphores_wait_info.pSemaphores, semaphores_wait_info.semaphoreCount) <<
      ", pValues:" << print_list(semaphores_wait_info.pValues, semaphores_wait_info.semaphoreCount);
  os << '}';
  return os;
}

#endif

} // namespace vk

#ifdef CWDEBUG
#include "LogicalDevice.h"
#include <vk_mem_alloc.h>

std::ostream& operator<<(std::ostream& os, VmaAllocationInfo const& vma_allocation_info)
{
  os << '{';
  os << "memoryType:" << vma_allocation_info.memoryType <<
      ", deviceMemory:" << vma_allocation_info.deviceMemory <<
      ", offset:" << vma_allocation_info.offset <<
      ", size:" << vma_allocation_info.size <<
      ", pMappedData:" << vma_allocation_info.pMappedData <<
      ", pUserData:" << vma_allocation_info.pUserData <<
      ", pName:" << debug::print_string(vma_allocation_info.pName);
  os << '}';
  return os;
}

std::ostream& operator<<(std::ostream& os, VmaAllocationCreateInfo const& vma_allocation_create_info)
{
  vulkan::LogicalDevice const* logical_device = debug::set_device::get_pword_value(os);
  // Use the I/O manipulator debug::set_device(logical_device) before writing a VmaAllocationCreateInfo to an ostream.
  ASSERT(logical_device);
  os << '{';
  os << "flags:" << vma_allocation_create_info.flags <<
      ", usage:" << vma_allocation_create_info.usage <<
      ", requiredFlags:" << vma_allocation_create_info.requiredFlags <<
      ", preferredFlags:" << vma_allocation_create_info.preferredFlags <<
      ", memoryTypeBits:" << utils::print_using(vma_allocation_create_info.memoryTypeBits, logical_device->memory_type_bits_printer()) <<
      ", pool:" << vma_allocation_create_info.pool <<
      ", pUserData:" << vma_allocation_create_info.pUserData <<
      ", priority:" << vma_allocation_create_info.priority;
  os << '}';
  return os;
}
#endif // CWDEBUG
