#pragma once

#include <vulkan/vulkan.hpp>
#include <type_traits>

namespace vk {

namespace details {

template<typename>
struct is_vk_flags : public std::false_type {};

template<typename BitType>
struct is_vk_flags<vk::Flags<BitType>> : public std::true_type
{
  using flags_type = vk::Flags<BitType>;
};

template<typename Flags>
concept ConceptIsVkFlags = is_vk_flags<Flags>::value && std::is_same_v<Flags, typename is_vk_flags<Flags>::flags_type>;

template<typename FE>
concept ConceptHasToString = requires(FE flags_or_enum)
{
  ::vk::to_string(flags_or_enum);
};

template<typename FE>
concept ConceptVkFlagsWithToString =
  (ConceptIsVkFlags<FE> || std::is_enum_v<FE>) && ConceptHasToString<FE>;

} // namespace details

// Print enum (class) types and Flags using vk::to_string.
template<typename Flags>
std::ostream& operator<<(std::ostream& os, Flags flags) requires details::ConceptVkFlagsWithToString<Flags>
{
  os << to_string(flags);
  return os;
}

} // namespace vk

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
DECLARE_OSTREAM_INSERTER(DeviceQueueCreateInfo)
DECLARE_OSTREAM_INSERTER(DeviceCreateInfo)
#if 0
DECLARE_OSTREAM_INSERTER(QueueFlagBits)
DECLARE_OSTREAM_INSERTER(SurfaceCapabilitiesKHR)
DECLARE_OSTREAM_INSERTER(SurfaceFormatKHR)
DECLARE_OSTREAM_INSERTER(SwapchainCreateInfoKHR)
#endif

} // namespace vk

#undef DECLARE_OSTREAM_INSERTER
