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
DECLARE_OSTREAM_INSERTER(QueueFamilyProperties)
DECLARE_OSTREAM_INSERTER(ExtensionProperties)
DECLARE_OSTREAM_INSERTER(PhysicalDeviceProperties)
DECLARE_OSTREAM_INSERTER(SurfaceCapabilitiesKHR)
DECLARE_OSTREAM_INSERTER(SurfaceFormatKHR)
DECLARE_OSTREAM_INSERTER(SwapchainCreateInfoKHR)
DECLARE_OSTREAM_INSERTER(ImageSubresourceRange)

} // namespace vk

#undef DECLARE_OSTREAM_INSERTER
