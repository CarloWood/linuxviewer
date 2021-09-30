#include "sys.h"
#include "LogicalDevice.h"
#include "PhysicalDeviceFeatures.h"
#include "DeviceCreateInfo.h"
#include "Application.h"
#include "debug.h"

namespace vulkan {

void LogicalDevice::prepare(vk::Instance vulkan_instance, DispatchLoader& dispatch_loader, task::VulkanWindow const* window_task_ptr)
{
  DoutEntering(dc::vulkan, "vulkan::LogicalDevice::prepare(" << vulkan_instance << ", dispatch_loader, " << (void*)window_task_ptr << ")");

  PhysicalDeviceFeatures physical_device_features;
  prepare_physical_device_features(physical_device_features);
  DeviceCreateInfo device_create_info(physical_device_features);
  prepare_logical_device(device_create_info);

  Dout(dc::vulkan, "Requested device_create_info: " << device_create_info << " [" << this << "]");

  if (device_create_info.has_queue_flag(QueueFlagBits::ePresentation))
    device_create_info.addDeviceExtentions({ VK_KHR_SWAPCHAIN_EXTENSION_NAME });

  auto vh_physical_devices = vulkan_instance.enumeratePhysicalDevices();
  auto const& required_extensions = device_create_info.device_extensions();
  for (auto const& vh_physical_device : vh_physical_devices)
  {
    QueueFamilies queue_families(vh_physical_device, window_task_ptr->vh_surface());
    if (queue_families.is_compatible_with(device_create_info, m_queue_replies))
    {
      if (!find_missing_extensions(required_extensions, vh_physical_device.enumerateDeviceExtensionProperties()).empty())
        continue;

      // Use the first compatible device.
      m_vh_physical_device = vh_physical_device;
      break;
    }
  }

  if (!m_vh_physical_device)
    THROW_ALERT("Could not find a physical device (GPU) that supports vulkan with the following requirements: [CREATE_INFO]", AIArgs("[CREATE_INFO]", device_create_info));

#ifdef CWDEBUG
  Dout(dc::vulkan, "Physical Device Properties:");
  {
    debug::Mark mark;
    auto properties = m_vh_physical_device.getProperties();
    Dout(dc::vulkan, properties);
  }
  Dout(dc::vulkan, "Physical Device Features:");
  {
    debug::Mark mark;
    auto features = m_vh_physical_device.getFeatures();
    Dout(dc::vulkan, features);
  }
  Dout(dc::vulkan, "Physical Device Extension Properties:");
  {
    debug::Mark mark;
    auto extension_properties_list = m_vh_physical_device.enumerateDeviceExtensionProperties();
    for (auto&& extension_properties : extension_properties_list)
      Dout(dc::vulkan, extension_properties);
  }
#endif

  // Construct a vector with the priority of each queue, per given queue family.
  std::map<GPU_queue_family_handle, std::vector<float>> priorities_per_family;
  // A handy reference to the requests.
  utils::Vector<QueueRequest> const& queue_requests = std::as_const(device_create_info).get_queue_requests();
  for (QueueRequestIndex qri(0); qri < QueueRequestIndex(queue_requests.size()); ++qri)
  {
    // A handy reference to the request and reply.
    QueueRequest const& request = queue_requests[qri];
    QueueReply const& reply = m_queue_replies[qri];
    for (int q = 0; q < reply.number_of_queues(); ++q)
      priorities_per_family[reply.get_queue_family_handle()].push_back(request.priority);
  }

  // Include each queue family with any of the requested features - in the pQueueCreateInfos of device_create_info.
  std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;
  for (auto iter = priorities_per_family.begin(); iter != priorities_per_family.end(); ++iter)
  {
    vk::DeviceQueueCreateInfo queue_create_info;
    queue_create_info
      .setQueueFamilyIndex(iter->first.get_value())
      .setQueuePriorities(iter->second);
    queue_create_infos.push_back(queue_create_info);
  }
  device_create_info.setQueueCreateInfos(queue_create_infos);

  Dout(dc::vulkan, "Calling m_vh_physical_device.createDevice(" << device_create_info << ")");
  m_device = m_vh_physical_device.createDeviceUnique(device_create_info);
  // For greater performance, immediately after creating a vulkan device, inform the extension loader.
  dispatch_loader.load(vulkan_instance, *m_device);

#ifdef CWDEBUG
  // Set the debug name of the device.
  // Note: when not using -DVULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1 this requires the dispatch_loader to be initialized (see the line above).
  vk::DebugUtilsObjectNameInfoEXT name_info(
    vk::ObjectType::eDevice,
    (uint64_t)static_cast<VkDevice>(*m_device),
    device_create_info.debug_name()
  );
  m_device->setDebugUtilsObjectNameEXT(name_info);
#endif
}

} // namespace vulkan

namespace task {

char const* LogicalDevice::state_str_impl(state_type run_state) const
{
  switch (run_state)
  {
    AI_CASE_RETURN(LogicalDevice_wait_for_window);
    AI_CASE_RETURN(LogicalDevice_create);
    AI_CASE_RETURN(LogicalDevice_done);
  }
  AI_NEVER_REACHED;
}

void LogicalDevice::multiplex_impl(state_type run_state)
{
  switch (run_state)
  {
    case LogicalDevice_wait_for_window:
      m_root_window->m_window_created_event.register_task(this, window_available_condition);
      set_state(LogicalDevice_create);
      wait(window_available_condition);
      break;
    case LogicalDevice_create:
      m_application->create_device(std::move(m_logical_device), m_root_window);
      set_state(LogicalDevice_done);
      [[fallthrough]];
    case LogicalDevice_done:
      finish();
      break;
  }
}

} // namespace task
