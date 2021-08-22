#include "sys.h"
#include "Device.h"
#include "DeviceCreateInfo.h"
#include "CommandPoolCreateInfo.h"
#include "QueueFamilyProperties.h"
#include "QueueReply.h"
#include "ExtensionLoader.h"
#include "find_missing_extensions.h"
#include "utils/Vector.h"
#include "utils/Array.h"
#include "utils/log2.h"
#include "utils/BitSet.h"
#include "utils/AIAlert.h"
#include "utils/MultiLoop.h"
#include "utils/almost_equal.h"
#include "debug.h"
#include <magic_enum.hpp>
#include <algorithm>
#include <utility>
#ifdef CWDEBUG
#include "debug_ostream_operators.h"
#include <sstream>
#endif

namespace vulkan {

#if 0
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
#endif

// The collection of queue family properties for a given physical device.
class QueueFamilies
{
 private:
  utils::Vector<QueueFamilyProperties, GPU_queue_family_handle> m_queue_families;

 public:
  // Check that for every required feature in device_create_info, there is at least one queue family that supports it.
  bool is_compatible_with(DeviceCreateInfo const& device_create_info, utils::Vector<QueueReply, QueueRequestIndex>& queue_replies)
  {
    DoutEntering(dc::vulkan, "QueueFamilies::is_compatible_with()");

    // Construct all available QueueFlags from m_queue_families.
    QueueFlags supported_queue_flags = {};
    for (auto const& queue_family_properties : m_queue_families)
      supported_queue_flags |= queue_family_properties.get_queue_flags();

    // Check if all the requested features are supported by at least one queue.
    QueueFlags unsupported_features = device_create_info.get_queue_flags() & ~supported_queue_flags;
    if (unsupported_features)
    {
      Dout(dc::vulkan, "Required feature(s) " << unsupported_features << " is/are not supported by any family queue.");
      return false;
    }

    // Run over all queue requests.
    utils::Vector<QueueRequest> queue_requests = device_create_info.get_queue_requests();
    size_t const number_of_requests = queue_requests.size();
    utils::Vector<std::vector<GPU_queue_family_handle>, QueueRequestIndex> queue_families_by_request(number_of_requests);
    QueueRequestIndex queue_request_index(0);    // index into queue_families_by_request.
    for (auto&& queue_request : queue_requests)
    {
      uint32_t maxQueueCount = 0;
      // Run over all supported queue families.
      GPU_queue_family_handle handle(0);
      for (auto const& queue_family_properties : m_queue_families)
      {
        QueueFlags required_but_unsupported = queue_request.queue_flags & ~queue_family_properties.get_queue_flags();
        if (!required_but_unsupported)
        {
          // We found a match between a request and a family.
          Dout(dc::vulkan, "QueueFamilyIndex " << handle << " supports request " << queue_request.queue_flags << ".");
          queue_families_by_request[queue_request_index].push_back(handle);
          if (queue_family_properties.queueCount > maxQueueCount)
            maxQueueCount = queue_family_properties.queueCount;
        }
        ++handle;
      }
      if (queue_families_by_request[queue_request_index].empty())
      {
        Dout(dc::vulkan, "No queue family has support for all of " << queue_request.queue_flags << ".");
        return false;
      }
      if (queue_request.max_number_of_queues == 0)      // A value of 0 means: as many as possible, so set R equal to the number of queues that this family supports.
        queue_request.max_number_of_queues = maxQueueCount;
      ++queue_request_index;
    }

    //-------------------------------------------------------------------------
    // Now hand out the queues in a smart way.

    // The queue_replies vector must be empty.
    ASSERT(queue_replies.empty());

    // queue_replies is filled, indexed by request; so we will have as many results (replies) as requests.
    queue_replies.resize(number_of_requests);

    // At this point we know that each request matches at least one queue family.
    // For example, assume we have four queue families (A, B, C and D) and five requests (0, 1, 2, 3, and 4).
    // Then the requests (0-4) could respectively match 0:A, 1:C, 2:ABC, 3:BCD, 4:ABCD:
    //
    //      0 -------> A ⎫              ⎫
    //                 B ⎬ ⎫ <------- 2 |______ 4
    //      1 -------> C ⎭ ⎬ <------- 3 |
    //                 D   ⎭            ⎭
    //
    // The below algorithm tries to return as many *different* queue families, so that
    // as many queues as possible can be created.
    //
    // Obviously, in the above example, request 0 is assigned family A because that is
    // the only family that is a match. Likewise 1 is assigned C. Therefore 2 will
    // have the preference for B because A and C are already used and 3 then has the
    // preference for D. However, then 4 is forced to use a family that was already
    // assigned. What we want in that case to have enough queues in that family to
    // satisfy both. If this is not possible then we want the minimize some penalty
    // function: the sum over all requests r of P_r * (R_r - H_r) / H_r,
    // where P_r is the priority of the request, R_r is the number of requested
    // queues, and H_r the number of handed out queues.
    //
    // Note that when H_r is zero, this function is undefined (read: plus infinity)
    // which will always be rejected as configuration: every requests gets at
    // least one queue. Also, if a priority is low then the weight of its penalty
    // term is low. Finally, if a request gets all requested queues (H_r == R_r)
    // then that terms completely drops out (is zero), while if H_r is almost
    // equal to R_r then the penalty per not-handed-out queue is proportional
    // with ~1/R (it is less bad to take away one queue from 16 then that it is
    // to take away one queue from 2).
    //
    // In te above example we would have:
    //
    // queue_families_by_request[0] = { A }
    // queue_families_by_request[1] = { C }
    // queue_families_by_request[2] = { A, B, C }
    // queue_families_by_request[3] = { B, C, D }
    // queue_families_by_request[4] = { A, B, C, D }
    //                           ^
    //                           |
    //                     qri = r: which request it is.
    //
    // This gives rize to 3 * 3 * 4 = 36 possible configurations.

    float minimum_penalty = std::numeric_limits<float>::max();
    // Run over all possible configurations.
    // We have number_of_requests nested for loops.
    // ml_qf are for loops that run over the Queue Families (one loop per request).
    for (MultiLoop ml_qf(number_of_requests); !ml_qf.finished(); ml_qf.next_loop())
    {
      // No code should go here!
      while (true)
      {
        // Each loop counter runs from 0 till queue_families_by_request[qri_qf].size(); where qri_qf is *ml_qf, the current loop.
        QueueRequestIndex const qri_qf(*ml_qf);
        if (ml_qf() >= queue_families_by_request[qri_qf].size())
          break;
        if (ml_qf.inner_loop())
        {
          // The easiest is to just brute force this: lets run over all possible ways to hand out the queues.
          // ml_h are for loops that run over the number of handed out queues (one loop per request).
          for (MultiLoop ml_h(number_of_requests, 1); !ml_h.finished(); ml_h.next_loop())
          {
            // No code should go here! (but a declaration is ok)
            while (true)
            {
              // Each loop counter runs from 1 till m_queue_families[qfh].queueCount, where qfh is the queue family handle
              // corresponding to request qri_h: queue_families_by_request[qri_h][ml_qf[qri_h]],
              // (e.g. for qri_h == 3, ml_qf[3] runs from 0..2 and queue_families_by_request[3][0..2] = { B, C, D }, so qfh would be B, C or D).
              // where qri_h is the queue request index corresponding to this loop: *ml_h.
              // Hence the maximum value of this loop can be m_queue_families[queue_families_by_request[*ml_h][ml_qf[*ml_h]]].queueCount.
              // However, it is also limited by requested number of queues for this request: queue_requests[r].max_number_of_queues,
              // aka queue_requests[*ml_h].max_number_of_queues.
              QueueRequestIndex const qri_h(*ml_h);
              GPU_queue_family_handle qfh = queue_families_by_request[qri_h][ml_qf[*ml_h]];
              if (ml_h() > std::min(
                  m_queue_families[qfh].queueCount,
                  queue_requests[qri_h].max_number_of_queues))
                break;

              // If different requests were given the same queue family, then it is still possible
              // that we handed out too many queues, eg both did get queueCount, oops.
              // It is too complicated to incorporate that into the limit above. Instead just do
              // a check here if the current loop went over that value, and if so - break out of this loop.
              uint32_t H_qfh = 0;       // The number of queues from queue family qfh that have been handed out.
              // Run over all requests that we already handed out queues to so far.
              for (QueueRequestIndex qri(0); qri <= qri_h; ++qri)
                if (queue_families_by_request[qri][ml_qf[qri.get_value()]] == qfh)      // Was this handed out from queue family qfh?
                  H_qfh += ml_h[qri.get_value()];                                       // Update the total count.

              if (H_qfh > m_queue_families[qfh].queueCount)             // Did we hand out more queues than the queue family has?
              {
                ml_h.breaks(1);                                         // Normal break (of current loop).
                break;                                                  // Return control to MultiLoop.
              }

              if (ml_h.inner_loop())
              {
                // Now lets calculate the penalty of this configuration.
                float penalty = 0;
                // Run over all requests.
                for (QueueRequestIndex qri(0); qri < QueueRequestIndex(number_of_requests); ++qri)
                {
                  float P = queue_requests[qri].priority;
                  int R = queue_requests[qri].max_number_of_queues;
                  int H = ml_h[qri.get_value()];
                  penalty += P * (R - H) / H;
                }
#ifdef CWDEBUG
                // Print debug information about the current configuration.
                std::ostringstream oss;
                char const* prefix = "";
                for (int r = 0; r < number_of_requests; ++r)
                {
                  oss << prefix << "request: " << r << ", queue family: " << ml_qf[r] << ", handed out: " << ml_h[r];
                  prefix = "; ";
                }
                Dout(dc::vulkan, "Configuration: {" << oss.str() << "}, penalty: " << penalty);
#endif
                if (penalty < minimum_penalty)
                {
                  // We found a better configuration.
                  minimum_penalty = penalty;

                  // Fill in the replies for this configuration: for each request: queue family (handle) and number of handed out queues.
                  for (QueueRequestIndex qri(0); qri < QueueRequestIndex(number_of_requests); ++qri)
                  {
                    GPU_queue_family_handle qfh = queue_families_by_request[qri][ml_qf[qri.get_value()]];
                    queue_replies[qri] = QueueReply(qfh, ml_h[qri.get_value()]);
                  }
                }
              }
              // Each loop counter starts at 1.
              ml_h.start_next_loop_at(1);
            }
          }
        }
        // Each loop counter starts at 0.
        ml_qf.start_next_loop_at(0);
      }
    }

#ifdef CWDEBUG
    Dout(dc::vulkan, "Result:");
    debug::Mark mark;
    for (QueueRequestIndex qri(0); qri < QueueRequestIndex(number_of_requests); ++qri)
    {
      Dout(dc::vulkan, "Request: " << device_create_info.get_queue_requests()[qri] << " ---> Reply: " << queue_replies[qri]);
    }

    // This function must give one Reply for each Request.
    bool all_replies_are_defined = true;
    for (QueueReply const& reply : queue_replies)
      if (reply.get_queue_family_handle().undefined())
      {
        all_replies_are_defined = false;
        break;
      }
    ASSERT(all_replies_are_defined);
#endif
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

  GPU_queue_family_handle handle(0);
  for (auto const& queueFamily : queueFamilies)
  {
    bool const presentation_support = physical_device.getSurfaceSupportKHR(handle.get_value(), surface);
    m_queue_families.emplace_back(queueFamily, presentation_support);
    ++handle;
  }
}

void Device::setup(vk::Instance vulkan_instance, ExtensionLoader& extension_loader, vk::SurfaceKHR surface, DeviceCreateInfo&& device_create_info)
{
  DoutEntering(dc::vulkan, "vulkan::Device::setup(" << vulkan_instance << ", " << surface << ", " << device_create_info << ")");

  // Use default if empty.
  if (std::as_const(device_create_info).get_queue_requests().empty())
    device_create_info.set_default_queue_requests();

  if (device_create_info.has_queue_flag(QueueFlagBits::ePresentation))
    device_create_info.addDeviceExtentions({ VK_KHR_SWAPCHAIN_EXTENSION_NAME });

  auto vh_physical_devices = vulkan_instance.enumeratePhysicalDevices();
  auto const& required_extensions = device_create_info.device_extensions();
  for (auto const& vh_physical_device : vh_physical_devices)
  {
    QueueFamilies queue_families(vh_physical_device, surface);
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
  m_uvh_device = m_vh_physical_device.createDeviceUnique(device_create_info);
  // For greater performance, immediately after creating a vulkan device, inform the extension loader.
  extension_loader.setup(vulkan_instance, *m_uvh_device);

#ifdef CWDEBUG
  // Set the debug name of the device.
  // Note: when not using -DVULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1 this requires the extension_loader to be initialized (see the line above).
  vk::DebugUtilsObjectNameInfoEXT name_info(
    vk::ObjectType::eDevice,
    (uint64_t)static_cast<VkDevice>(*m_uvh_device),
    device_create_info.debug_name()
  );
  m_uvh_device->setDebugUtilsObjectNameEXT(name_info);
#endif
}

Device::~Device()
{
  DoutEntering(dc::vulkan, "vulkan::Device::~Device()");
}

void Device::create_command_pool(CommandPoolCreateInfo const& command_pool_create_info)
{
  m_uvh_command_pool = m_uvh_device->createCommandPoolUnique(command_pool_create_info);

#ifdef CWDEBUG
  // Set the debug name of the command pool.
  vk::DebugUtilsObjectNameInfoEXT name_info(
    vk::ObjectType::eCommandPool,
    (uint64_t)static_cast<VkCommandPool>(*m_uvh_command_pool),
    command_pool_create_info.debug_name()
  );
  m_uvh_device->setDebugUtilsObjectNameEXT(name_info);
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
