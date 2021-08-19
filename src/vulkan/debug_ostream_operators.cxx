#include "sys.h"

#ifdef CWDEBUG
#include "debug_ostream_operators.h"
//#include "../data_types/UUID.h"
#include <magic_enum.hpp>
#include <iostream>
#include <iomanip>
#include "debug.h"

namespace {

std::string print_version(uint32_t version)
{
  std::string version_str = std::to_string(VK_API_VERSION_MAJOR(version));
  version_str += '.';
  version_str += std::to_string(VK_API_VERSION_MINOR(version));
  version_str += '.';
  version_str += std::to_string(VK_API_VERSION_PATCH(version));
  return version_str;
}

} // namespace

namespace vk {

using NAMESPACE_DEBUG::print_string;

std::ostream& operator<<(std::ostream& os, ApplicationInfo const& application_info)
{
  os << '{';
  os << "allowDuplicate:" << std::boolalpha << application_info.allowDuplicate << ", ";
  os << "pApplicationName:" << print_string(application_info.pApplicationName) << ", ";
  os << "applicationVersion:" << print_version(application_info.applicationVersion) << ", ";
  os << "pEngineName:" << print_string(application_info.pEngineName) << ", ";
  os << "engineVersion:" << print_version(application_info.engineVersion) << ", ";
  os << "apiVersion:" << print_version(application_info.apiVersion) << '}';
  return os;
}

std::ostream& operator<<(std::ostream& os, InstanceCreateInfo const& instance_create_info)
{
  os << '{';
  os << "flags:" << static_cast<decltype(instance_create_info.flags)::MaskType>(instance_create_info.flags) << ", ";
  os << "pApplicationInfo:" << (void*)instance_create_info.pApplicationInfo << ", ";
  os << "enabledLayerCount:" << instance_create_info.enabledLayerCount << ", ";
  os << "ppEnabledLayerNames:<";
  for (int i = 0; i < instance_create_info.enabledLayerCount; ++i)
  {
    if (i > 0)
      os << ',';
    os << print_string(instance_create_info.ppEnabledLayerNames[i]);
  }
  os << ">, ";
  os << "enabledExtensionCount:" << instance_create_info.enabledExtensionCount << ", ";
  os << "ppEnabledExtensionNames:<";
  for (int i = 0; i < instance_create_info.enabledExtensionCount; ++i)
  {
    if (i > 0)
      os << ',';
    os << print_string(instance_create_info.ppEnabledExtensionNames[i]);
  }
  os << ">}";
  return os;
}

std::ostream& operator<<(std::ostream& os, DebugUtilsObjectNameInfoEXT const& debug_utils_object_name_info)
{
  os << '{';
  os << "objectType:" << to_string(debug_utils_object_name_info.objectType) << ", ";
  os << "objectHandle:" << std::hex << debug_utils_object_name_info.objectHandle << ", ";
  os << "pObjectName:" << print_string(debug_utils_object_name_info.pObjectName);
  os << '}';
  return os;
}

std::ostream& operator<<(std::ostream& os, DeviceCreateInfo const& device_create_info)
{
  os << '{';
  os << "pNext:" << device_create_info.pNext << ", ";
  os << "flags:" << static_cast<decltype(device_create_info.flags)::MaskType>(device_create_info.flags) << ", ";
  os << "pQueueCreateInfos:<";
  for (int i = 0; i < device_create_info.queueCreateInfoCount; ++i)
  {
    if (i > 0)
      os << ',';
    os << device_create_info.pQueueCreateInfos[i];
  }
  os << ">, ";
  os << "ppEnabledLayerNames:<";
  for (int i = 0; i < device_create_info.enabledLayerCount; ++i)
  {
    if (i > 0)
      os << ',';
    os << print_string(device_create_info.ppEnabledLayerNames[i]);
  }
  os << ">, ";
  os << "ppEnabledExtensionNames:<";
  for (int i = 0; i < device_create_info.enabledExtensionCount; ++i)
  {
    if (i > 0)
      os << ',';
    os << print_string(device_create_info.ppEnabledExtensionNames[i]);
  }
  os << ">, pEnabledFeatures";
  if (device_create_info.pEnabledFeatures)
    os << "->" << *device_create_info.pEnabledFeatures;
  else
    os << ":nullptr";
  os << '}';
  return os;
}

std::ostream& operator<<(std::ostream& os, DeviceQueueCreateInfo const& device_queue_create_info)
{
  os << '{';
  os << "pNext:" << device_queue_create_info.pNext << ", ";
  os << "flags:" << static_cast<decltype(device_queue_create_info.flags)::MaskType>(device_queue_create_info.flags) << ", ";
  os << "queueFamilyIndex:" << device_queue_create_info.queueFamilyIndex << ", ";
  os << "queueCount: " << device_queue_create_info.queueCount << ", ";
  os << "pQueuePriorities:<";
  for (int i = 0; i < device_queue_create_info.queueCount; ++i)
  {
    if (i > 0)
      os << ',';
    os << device_queue_create_info.pQueuePriorities[i];
  }
  os << ">}";
  return os;
}

std::ostream& operator<<(std::ostream& os, PhysicalDeviceFeatures const& physical_device_features)
{
#define SHOW_IF_TRUE(var) \
  do { if (physical_device_features.var) { os << prefix << #var; prefix = ", "; } } while(0)
  char const* prefix = "{<";
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
  os << ">}";
  return os;
#undef SHOW_IF_TRUE
}

std::ostream& operator<<(std::ostream& os, QueueFamilyProperties const& queue_family_properties)
{
  os << '{';
  os << "queueFlags:" << queue_family_properties.queueFlags << ", ";
  os << "queueCount:" << queue_family_properties.queueCount << ", ";
  os << "timestampValidBits:" << queue_family_properties.timestampValidBits << ", ";
  os << "minImageTransferGranularity:" << queue_family_properties.minImageTransferGranularity;
  os << '}';
  return os;
}

std::ostream& operator<<(std::ostream& os, Extent3D const& extend_3D)
{
  os << '{';
  os << "width:" << extend_3D.width << ", ";
  os << "height:" << extend_3D.height << ", ";
  os << "depth:" << extend_3D.depth;
  os << '}';
  return os;
}

std::ostream& operator<<(std::ostream& os, Extent2D const& extend_2D)
{
  os << '{';
  os << "width:" << extend_2D.width << ", ";
  os << "height:" << extend_2D.height;
  os << '}';
  return os;
}

std::ostream& operator<<(std::ostream& os, QueueFlagBits const& queue_flag_bit)
{
  return os << magic_enum::enum_name(queue_flag_bit);
}

std::ostream& operator<<(std::ostream& os, ExtensionProperties const& extension_properties)
{
  os << '{';
  os << "extensionName:" << print_string(extension_properties.extensionName) << ", ";
  os << "specVersion:" << extension_properties.specVersion;
  os << '}';
  return os;
}

std::ostream& operator<<(std::ostream& os, PhysicalDeviceProperties const& physical_device_properties)
{
  os << '{';
  os << "apiVersion:" << print_version(physical_device_properties.apiVersion) << ", ";
  os << "driverVersion:" << print_version(physical_device_properties.driverVersion) << ", ";
  os << "vendorID:0x" << std::hex << physical_device_properties.vendorID << ", ";
  os << "deviceID:0x" << std::hex << physical_device_properties.deviceID << ", ";
  os << "deviceType:" << to_string(physical_device_properties.deviceType) << ", ";
  os << "deviceName:" << print_string(physical_device_properties.deviceName) << ", ";
//  os << "pipelineCacheUUID:" << UUID(reinterpret_cast<char const*>(static_cast<uint8_t const*>(physical_device_properties.pipelineCacheUUID))) << ", ";
//    VULKAN_HPP_NAMESPACE::PhysicalDeviceLimits                                   limits            = {};
//    VULKAN_HPP_NAMESPACE::PhysicalDeviceSparseProperties                         sparseProperties  = {};
  os << '}';
  return os;
}

} // namespace vk

#endif // CWDEBUG
