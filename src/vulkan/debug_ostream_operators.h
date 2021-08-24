#pragma once

#include <vulkan/vulkan.hpp>
#include <type_traits>

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

namespace vk {

// ostream inserters for objects in namespace vk:
std::ostream& operator<<(std::ostream& os, ApplicationInfo const& application_info);
std::ostream& operator<<(std::ostream& os, InstanceCreateInfo const& instance_create_info);
std::ostream& operator<<(std::ostream& os, DebugUtilsObjectNameInfoEXT const& debug_utils_object_name_info);

// Print enum (class) types and Flags using vk::to_string.
template<typename Flags>
std::ostream& operator<<(std::ostream& os, Flags flags) requires details::ConceptVkFlagsWithToString<Flags>
{
  os << to_string(flags);
  return os;
}

std::ostream& operator<<(std::ostream& os, DeviceCreateInfo const& device_create_info);
std::ostream& operator<<(std::ostream& os, DeviceQueueCreateInfo const& device_queue_create_info);
std::ostream& operator<<(std::ostream& os, PhysicalDeviceFeatures const& physical_device_features);
std::ostream& operator<<(std::ostream& os, QueueFamilyProperties const& queue_family_properties);
std::ostream& operator<<(std::ostream& os, Extent3D const& extend_3D);
std::ostream& operator<<(std::ostream& os, Extent2D const& extend_2D);
std::ostream& operator<<(std::ostream& os, QueueFlagBits const& queue_flag_bit);
std::ostream& operator<<(std::ostream& os, ExtensionProperties const& extension_properties);
std::ostream& operator<<(std::ostream& os, PhysicalDeviceProperties const& physical_device_properties);
std::ostream& operator<<(std::ostream& os, SurfaceCapabilitiesKHR const& surface_capabilities);
std::ostream& operator<<(std::ostream& os, SurfaceFormatKHR const& surface_format);
std::ostream& operator<<(std::ostream& os, SwapchainCreateInfoKHR const& swap_chain_create_info);

} // namespace vk
