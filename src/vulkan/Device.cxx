#include "sys.h"
#include "Device.h"
#include "DeviceCreateInfo.h"
#include "QueueFamilyProperties.h"
#include "ExtensionLoader.h"
#include "utils/Vector.h"
#include "utils/Array.h"
#include "utils/log2.h"
#include "utils/BitSet.h"
#include "utils/AIAlert.h"
#include "debug.h"
#include <magic_enum.hpp>
#ifdef CWDEBUG
#include "debug_ostream_operators.h"
#endif

namespace vulkan {

// vk::QueueFlagBits is an enum class describing the different types of queue families that exist
// in VulkanCore; they do not include extensions.
//
// We need to map those to an Array, so each value has to be associated with an ArrayIndex.
// ===> Note: variables of this type will be abbreviated with cqfi! <===
using core_queue_family_index = utils::ArrayIndex<vk::QueueFlagBits>;

// The size of the array.
//
// These are all the theoretically possible queue families supported in Vulkan Core;
// they do unfortunately not include extensions, nor do will every GPU support all of them.
static constexpr size_t number_of_core_queue_families = magic_enum::enum_count<vk::QueueFlagBits>();

// For the translation from index to enum we can use magic_enum...
constexpr vk::QueueFlagBits cqfi_to_QueueFlagBits(core_queue_family_index index)
{
  return magic_enum::enum_value<vk::QueueFlagBits>(index.get_value());
}

// but not the other way around. Therefore we'll rely on the fact that the enum
// values are powers of two. Lets first make sure this is the case:
template<typename E>
static constexpr bool is_powers_of_two()
{
  for (int i = 0; i < (int)magic_enum::enum_count<E>(); ++i)
    if (magic_enum::enum_integer(magic_enum::enum_value<E>(i)) != 1 << i)
      return false;
  return true;
}
static_assert(is_powers_of_two<vk::QueueFlagBits>(), "Elements of vk::QueueFlagBits are expected to be powers of two :(");

// Then we can use a simple function for the translation from enum to index.
constexpr core_queue_family_index QueueFlagBits_to_cqfi(vk::QueueFlagBits flag)
{
  vk::QueueFlags::MaskType bit = static_cast<vk::QueueFlags::MaskType>(flag);
  return core_queue_family_index(utils::log2(bit));
}

// Furthermore we have queue family handles (uint32_t) that are the index
// into the vector returned by vk::PhysicalDevice::getQueueFamilyProperties().
struct GPU_queue_family_handle_category {};
using GPU_queue_family_handle = utils::VectorIndex<GPU_queue_family_handle_category>;

// The collection of queue family properties for a given physical device.
class QueueFamilies
{
 private:
  utils::Vector<vulkan::QueueFamilyProperties, GPU_queue_family_handle> m_queue_families;

 public:
  // Check that for every required feature in device_create_info, there is at least one queue family that supports it.
  bool is_compatible_with(vulkan::DeviceCreateInfo const& device_create_info, std::vector<GPU_queue_family_handle>& queue_families_with_support)
  {
    DoutEntering(dc::vulkan, "QueueFamilies::is_compatible_with()");

    vk::QueueFlags core_flags = {};
    bool presentation_support = false;

    for (auto const& queue_family_properties : m_queue_families)
    {
      core_flags |= queue_family_properties.queueFlags;
      presentation_support |= queue_family_properties.has_presentation_support();
    }

    // Check if presentation is required and available.
    if (device_create_info.get_presentation_flag() && !presentation_support)
    {
      Dout(dc::vulkan, "Required feature Presentation is not supported by any family queue.");
      return false;
    }

    // Check if all the requested core features are available.
    vk::QueueFlags unsupported_features = device_create_info.get_queue_flags() & ~core_flags;
    if (unsupported_features)
    {
      Dout(dc::vulkan, "Required feature(s) " << unsupported_features << " is/are not supported by any family queue.");
      return false;
    }

    GPU_queue_family_handle handle;
    handle.set_to_zero();
    for (auto const& queue_family_properties : m_queue_families)
    {
      bool presentation_required_and_supported = device_create_info.get_presentation_flag() && queue_family_properties.has_presentation_support();
      vk::QueueFlags core_required_and_supported = device_create_info.get_queue_flags() & queue_family_properties.queueFlags;
      if (presentation_required_and_supported || core_required_and_supported)
      {
#ifdef CWDEBUG
        Dout(dc::vulkan|continued_cf, "Adding queue queueFamilyIndex " << handle << " because ");
        char const* prefix = "";
        if (presentation_required_and_supported)
        {
          Dout(dc::continued, "it supports presentation");
          prefix = " and ";
        }
        if (core_required_and_supported)
        {
          Dout(dc::continued, prefix << "it supports " << core_required_and_supported);
        }
        Dout(dc::finish, ".");
#endif
        queue_families_with_support.push_back(handle);
      }
      ++handle;
    }

    return true;
  }

  QueueFamilies(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface);
};

QueueFamilies::QueueFamilies(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface)
{
  DoutEntering(dc::vulkan, "QueueFamilies::QueueFamilies(" << physical_device << ", " << surface << ")");

  std::vector<vk::QueueFamilyProperties> queueFamilies = physical_device.getQueueFamilyProperties();

  Dout(dc::vulkan, "getQueueFamilyProperties() returned " << queueFamilies.size() << " families:");
#ifdef CWDEBUG
  debug::Mark mark;
#endif

  GPU_queue_family_handle handle;
  handle.set_to_zero();
  for (auto const& queueFamily : queueFamilies)
  {
    bool const presentation_support = physical_device.getSurfaceSupportKHR(handle.get_value(), surface);
    m_queue_families.emplace_back(queueFamily, presentation_support);
    ++handle;
  }
}

void Device::setup(vk::Instance vulkan_instance, vulkan::ExtensionLoader& extension_loader, vk::SurfaceKHR surface, vulkan::DeviceCreateInfo&& device_create_info)
{
  DoutEntering(dc::vulkan, "Device::setup(" << vulkan_instance << ", " << surface << ", " << device_create_info << ")");

  if (device_create_info.get_presentation_flag())
    device_create_info.addDeviceExtentions({ VK_KHR_SWAPCHAIN_EXTENSION_NAME });

  auto physical_devices = vulkan_instance.enumeratePhysicalDevices();
  std::vector<GPU_queue_family_handle> queue_families_with_support;
  for (auto const& physical_device : physical_devices)
  {
    QueueFamilies queue_families(physical_device, surface);
    if (queue_families.is_compatible_with(device_create_info, queue_families_with_support))
    {
      // Use the first compatible device.
      m_physical_device = physical_device;
      break;
    }
  }

  if (!m_physical_device)
    THROW_ALERT("Could not find a physical device (GPU) that supports vulkan with the following requirements: [CREATE_INFO]", AIArgs("[CREATE_INFO]", device_create_info));

#ifdef CWDEBUG
  Dout(dc::vulkan, "Physical Device Properties:");
  {
    debug::Mark mark;
    auto properties = m_physical_device.getProperties();
    Dout(dc::vulkan, properties);
  }
  Dout(dc::vulkan, "Physical Device Features:");
  {
    debug::Mark mark;
    auto features = m_physical_device.getFeatures();
    Dout(dc::vulkan, features);
  }
  Dout(dc::vulkan, "Physical Device Extension Properties:");
  {
    debug::Mark mark;
    auto extension_properties_list = m_physical_device.enumerateDeviceExtensionProperties();
    for (auto&& extension_properties : extension_properties_list)
      Dout(dc::vulkan, extension_properties);
  }
#endif

  // Include each queue family with any of the requested features - in the pQueueCreateInfos of device_create_info.
  float queue_priority = 1.0f;
  std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;
  for (GPU_queue_family_handle queue_family : queue_families_with_support)
  {
    vk::DeviceQueueCreateInfo queue_create_info;
    queue_create_info
      .setQueueFamilyIndex(queue_family.get_value())
      .setQueuePriorities(queue_priority);
    queue_create_infos.push_back(queue_create_info);
  }
  device_create_info.setQueueCreateInfos(queue_create_infos);

  Dout(dc::vulkan, "Calling m_physical_device.createDevice(" << device_create_info << ")");
  m_device_handle = m_physical_device.createDevice(device_create_info);
  // For greater performance, immediately after creating a vulkan device, inform the extension loader.
  extension_loader.setup(vulkan_instance, m_device_handle);

#ifdef CWDEBUG
  // Set the debug name of the device.
  // Note: when not using -DVULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1 this requires the extension_loader to be initialized (see the line above).
  vk::DebugUtilsObjectNameInfoEXT name_info(
    vk::ObjectType::eDevice,
    (uint64_t)static_cast<VkDevice>(m_device_handle),
    device_create_info.debug_name()
  );
  m_device_handle.setDebugUtilsObjectNameEXT(&name_info);
#endif
}

#ifdef CWDEBUG
void Device::print_on(std::ostream& os) const
{
  os << '{';
  os << "(Device*)" << (void*)this;
  os << '}';
}
#endif

} // namespace vulkan
