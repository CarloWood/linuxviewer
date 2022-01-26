#include "sys.h"
#include "debug/vulkan_print_on.h"
#include "CommandBuffer.h"
#include "Application.h"
#include "LogicalDevice.h"
#include "QueueFamilyProperties.h"
#include "QueueReply.h"
#include "infos/DeviceCreateInfo.h"
#include "vk_utils/find_missing_names.h"
#include "vk_utils/print_using_to_string.h"
#include "vk_utils/get_binary_file_contents.h"
#include "debug/DebugSetName.h"
#include "utils/is_power_of_two.h"
#include "utils/MultiLoop.h"
#include "debug.h"

namespace vulkan {

bool QueueFamilies::is_compatible_with(DeviceCreateInfo const& device_create_info, utils::Vector<QueueReply, QueueRequestIndex>& queue_replies)
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
  utils::Vector<std::vector<QueueFamilyPropertiesIndex>, QueueRequestIndex> queue_families_by_request(number_of_requests);
  QueueRequestIndex queue_request_index(0);    // index into queue_families_by_request.
  for (auto&& queue_request : queue_requests)
  {
    uint32_t max_queue_count = 0;
    // Run over all supported queue families.
    QueueFamilyPropertiesIndex queue_family(0);
    for (auto const& queue_family_properties : m_queue_families)
    {
      QueueFlags required_but_unsupported = queue_request.queue_flags & ~queue_family_properties.get_queue_flags();
      if (!required_but_unsupported)
      {
        // We found a match between a request and a family.
        Dout(dc::vulkan, "QueueFamilyIndex " << queue_family << " supports request " << queue_request.queue_flags << ".");
        queue_families_by_request[queue_request_index].push_back(queue_family);
        if (queue_family_properties.queueCount > max_queue_count)
          max_queue_count = queue_family_properties.queueCount;
      }
      ++queue_family;
    }
    if (queue_families_by_request[queue_request_index].empty())
    {
      Dout(dc::vulkan, "No queue family has support for all of " << queue_request.queue_flags << ".");
      return false;
    }
    if (queue_request.max_number_of_queues == 0)      // A value of 0 means: as many as possible, so set R equal to the number of queues that this family supports.
      queue_request.max_number_of_queues = max_queue_count;
    ++queue_request_index;
  }

  //-------------------------------------------------------------------------
  // Now hand out the queues in a smart way.

  // The queue_replies vector must be empty.
  ASSERT(queue_replies.empty());

  // queue_replies is filled, indexed by request; so we will have as many results (replies) as requests.
  {
    // Use swap instead of resize, because QueueReply contains an atomic and is therefore not moveable.
    utils::Vector<QueueReply, QueueRequestIndex> new_queue_replies(number_of_requests);
    queue_replies.swap(new_queue_replies);
  }

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
        // Start value of 0 is abused to mean "Combine with previous request".
        // The first request can never be combined, so the start value is 1.
        for (MultiLoop ml_h(number_of_requests, 1); !ml_h.finished(); ml_h.next_loop())
        {
          // No code should go here! (but a declaration is ok)
          while (true)
          {
            // Each loop counter runs from 1 till m_queue_families[queue_family].queueCount, where queue_family is the queue family index
            // corresponding to request qri_h: queue_families_by_request[qri_h][ml_qf[qri_h]],
            // (e.g. for qri_h == 3, ml_qf[3] runs from 0..2 and queue_families_by_request[3][0..2] = { B, C, D }, so queue_family would be B, C or D).
            // where qri_h is the queue request index corresponding to this loop: *ml_h.
            // Hence the maximum value of this loop can be m_queue_families[queue_families_by_request[*ml_h][ml_qf[*ml_h]]].queueCount.
            // However, it is also limited by requested number of queues for this request: queue_requests[r].max_number_of_queues,
            // aka queue_requests[*ml_h].max_number_of_queues.
            QueueRequestIndex const qri_h(*ml_h);
            QueueFamilyPropertiesIndex queue_family = queue_families_by_request[qri_h][ml_qf[*ml_h]];
            if (ml_h() > std::min(
                m_queue_families[queue_family].queueCount,
                queue_requests[qri_h].max_number_of_queues))
              break;

            bool combine = ml_h() == 0;
            if (combine)
            {
              ASSERT(!queue_requests[qri_h].combined_with.undefined());
              ASSERT(!qri_h.is_zero());
              ASSERT(queue_requests[qri_h].combined_with == qri_h - 1);
              ASSERT(queue_family == queue_families_by_request[qri_h - 1][ml_qf[*ml_h - 1]]);
            }

            // If different requests were given the same queue family, then it is still possible
            // that we handed out too many queues, eg both did get queueCount, oops.
            // It is too complicated to incorporate that into the limit above. Instead just do
            // a check here if the current loop went over that value, and if so - break out of this loop.
            uint32_t H_queue_family = 0;       // The number of queues from queue_family that have been handed out.
            // Run over all requests that we already handed out queues to so far.
            // Note how requests that are combined have a value ml_h[qri.get_value()] of zero, so
            // they do not contribute to the total count.
            for (QueueRequestIndex qri(0); qri <= qri_h; ++qri)
              if (queue_families_by_request[qri][ml_qf[qri.get_value()]] == queue_family)     // Was this handed out from queue_family?
                H_queue_family += ml_h[qri.get_value()];                                      // Update the total count.

            if (H_queue_family > m_queue_families[queue_family].queueCount)                   // Did we hand out more queues than the queue family has?
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
                if (H > 0)
                  penalty += P * (R - H) / H;
                else
                {
                  QueueRequestIndex qri0 = queue_requests[qri].combined_with;
                  float P0 = queue_requests[qri0].priority;
                  int R0 = queue_requests[qri0].max_number_of_queues;
                  H = ml_h[qri0.get_value()];
                  ASSERT(R <= R0);      // Requests that are combined with another must have a smaller number of max_number_of_queues.
                  if (H < R)
                    penalty += P * (R - H) / H;
                  // Give a reward for the fact that this actually combined the two requests.
                  penalty -= 0.4 * P0;
                }
              }
              if (penalty < minimum_penalty)
              {
                minimum_penalty = penalty;

                // For now, only print a new configuration when it is better than the previous one.
                // Otherwise this floods too much.
                // We found a better configuration.
#ifdef CWDEBUG
                // Print debug information about the current configuration.
                std::ostringstream oss;
                char const* prefix = "";
                for (QueueRequestIndex qri(0); qri < QueueRequestIndex(number_of_requests); ++qri)
                {
                  QueueFamilyPropertiesIndex queue_family = queue_families_by_request[qri][ml_qf[qri.get_value()]];
                  int H = ml_h[qri.get_value()];
                  if (H > 0)
                    oss << prefix << "{QF:" << queue_family << ", H:" << H << "}";
                  else
                    oss << prefix << "{combined with " << queue_requests[qri].combined_with << "}";
                  prefix = ", ";
                }
                Dout(dc::vulkan, "Configuration: <" << oss.str() << ">, penalty: " << penalty);
#endif

                // Fill in the replies for this configuration: for each request: queue family (index) and number of handed out queues.
                for (QueueRequestIndex qri(0); qri < QueueRequestIndex(number_of_requests); ++qri)
                {
                  QueueRequest const& request = queue_requests[qri];
                  QueueFamilyPropertiesIndex queue_family = queue_families_by_request[qri][ml_qf[qri.get_value()]];
                  QueueFlags queue_flags = request.queue_flags;
                  int H = ml_h[qri.get_value()];
                  QueueRequestIndex qri0;
                  if (H == 0)
                  {
                    qri0 = request.combined_with;
                    ASSERT(!qri0.undefined());
                  }
                  queue_replies[qri] = QueueReply(queue_family, H, queue_flags, request.windows, qri0);
                }
              }
            }
            // Each loop counter starts at 1, unless it can be combined with the previous request.
            // So this tests with this loop can be combined with the next loop, and if so lets
            // the next loop start with 0.
            bool cannot_be_combined =
              *ml_h == number_of_requests - 1 ||                                        // This is the last loop.
              queue_requests[qri_h + 1].combined_with.undefined() ||                    // The next request isn't combined.
              queue_families_by_request[qri_h + 1][ml_qf[*ml_h + 1]] != queue_family;   // The queue family of the next request isn't equal to the current queue family.
            ml_h.start_next_loop_at(cannot_be_combined ? 1 : 0);
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
    if (reply.get_queue_family().undefined())
    {
      all_replies_are_defined = false;
      break;
    }
  ASSERT(all_replies_are_defined);
#endif
  return true;
}

QueueFamilies::QueueFamilies(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface)
{
  DoutEntering(dc::vulkan, "QueueFamilies::QueueFamilies(" << physical_device << ", " << surface << ")");

  std::vector<vk::QueueFamilyProperties> queueFamilies = physical_device.getQueueFamilyProperties();

  Dout(dc::vulkan, "getQueueFamilyProperties() returned " << queueFamilies.size() << " families:");
#ifdef CWDEBUG
  debug::Mark mark;
#endif

  QueueFamilyPropertiesIndex queue_family(0);
  for (auto const& queueFamily : queueFamilies)
  {
    bool const presentation_support = physical_device.getSurfaceSupportKHR(queue_family.get_value(), surface);
    m_queue_families.emplace_back(queueFamily, presentation_support);
    ++queue_family;
  }
}

bool LogicalDevice::verify_presentation_support(vulkan::PresentationSurface const& surface) const
{
  DoutEntering(dc::vulkan, "LogicalDevice::verify_presentation_support(" << surface << ")");
  auto queue_family = surface.presentation_queue().queue_family();
  bool const presentation_support = m_vh_physical_device.getSurfaceSupportKHR(queue_family.get_value(), surface.vh_surface());
  Dout(dc::warning(!presentation_support), "The physical device " << m_vh_physical_device << " has no presentation support for the surface " << surface << "!");
  return presentation_support;
}

void LogicalDevice::prepare(
    vk::Instance vulkan_instance,
    DispatchLoader& dispatch_loader,
    task::SynchronousWindow const* window_task_ptr)
{
  DoutEntering(dc::vulkan, "vulkan::LogicalDevice::prepare(" << vulkan_instance << ", dispatch_loader, " << (void*)window_task_ptr << ")");

  // Get the queue family requirements from the user, using the virtual function prepare_logical_device
  // and enable the imagelessFramebuffer feature.
  vk::PhysicalDeviceFeatures2 features2 = {
    .features =
      // 1.0 features.
      { }
  };
  vk::StructureChain<DeviceCreateInfo, vk::PhysicalDeviceVulkan11Features, vk::PhysicalDeviceVulkan12Features> device_create_info_chain({},
      // 1.1 features.
      { },
      // 1.2 features.
      { .imagelessFramebuffer = true,           // Mandatory feature.
        .separateDepthStencilLayouts = true }   // Optional feature.
  );

  // Get the required physical device features from the user, using the virtual function prepare_physical_device_features.
  auto& features10 = features2.features;
  auto& features11 = device_create_info_chain.get<vk::PhysicalDeviceVulkan11Features>();
  auto& features12 = device_create_info_chain.get<vk::PhysicalDeviceVulkan12Features>();
  prepare_physical_device_features(features10, features11, features12);

  // Enforce mandatory features.
  if (!features12.imagelessFramebuffer)
  {
    Dout(dc::warning, "imagelessFramebuffer is mandatory!");
    features12.setImagelessFramebuffer(VK_TRUE);
  }

  // Link features11 and on also from features2, and print that.
  features2.setPNext(&features11);
  Dout(dc::vulkan, "Requested PhysicalDeviceFeatures: " << features2 << " [" << this << "]");

  DeviceCreateInfo& device_create_info = device_create_info_chain.get<DeviceCreateInfo>();
  device_create_info.setPEnabledFeatures(&features10);
  prepare_logical_device(device_create_info);

  Dout(dc::vulkan, "Requested QueueRequest's: " << device_create_info.get_queue_requests() << " [" << this << "]");

  if (device_create_info.has_queue_flag(QueueFlagBits::ePresentation))
    device_create_info.addDeviceExtentions({ VK_KHR_SWAPCHAIN_EXTENSION_NAME });

  auto required_extensions = device_create_info.device_extensions();

  auto vh_physical_devices = vulkan_instance.enumeratePhysicalDevices();
  for (auto const& vh_physical_device : vh_physical_devices)
  {
    QueueFamilies queue_families(vh_physical_device, window_task_ptr->vh_surface());
    if (queue_families.is_compatible_with(device_create_info, m_queue_replies))
    {
      auto extension_properties = vh_physical_device.enumerateDeviceExtensionProperties();
      std::vector<char const*> available_names;
      for (auto&& property : extension_properties)
        available_names.push_back(property.extensionName);

      // Check that every extension name in required_extensions is available.
      auto missing_extensions = vk_utils::find_missing_names(required_extensions, available_names);

      if (!missing_extensions.empty())
        continue;

      // Use the first compatible device.
      m_vh_physical_device = vh_physical_device;
      m_queue_families = std::move(queue_families);
      break;
    }
  }

  if (!m_vh_physical_device)
    THROW_ALERT("Could not find a physical device (GPU) that supports vulkan with the following requirements: [CREATE_INFO]", AIArgs("[CREATE_INFO]", device_create_info));

  // Check for optional features.
  Dout(dc::vulkan, "Physical Device Properties:");
  {
    debug::Mark mark;
    auto properties = m_vh_physical_device.getProperties();
    Dout(dc::vulkan, properties);
    m_non_coherent_atom_size = properties.limits.nonCoherentAtomSize;
  }
  Dout(dc::vulkan, "Physical Device Features:");
  {
#ifdef CWDEBUG
    debug::Mark mark;
#endif
    m_vh_physical_device.getFeatures2(&features2);
    m_supports_sampler_anisotropy = features10.samplerAnisotropy;
    m_supports_separate_depth_stencil_layouts = features12.separateDepthStencilLayouts;
    Dout(dc::vulkan, features2);
  }
#ifdef CWDEBUG
  Dout(dc::vulkan, "Physical Device Extension Properties:");
  {
    debug::Mark mark;
    auto extension_properties_list = m_vh_physical_device.enumerateDeviceExtensionProperties();
    for (auto&& extension_properties : extension_properties_list)
      Dout(dc::vulkan, extension_properties);
  }
#endif

  // Construct a vector with the priority of each queue, per given queue family.
  std::map<QueueFamilyPropertiesIndex, std::vector<float>> priorities_per_family;
  // A handy reference to the requests.
  utils::Vector<QueueRequest> const& queue_requests = std::as_const(device_create_info).get_queue_requests();
  for (auto qri = queue_requests.ibegin(); qri != queue_requests.iend(); ++qri)
  {
    // A handy reference to the request and reply.
    QueueRequest const& request = queue_requests[qri];
    QueueReply const& reply = m_queue_replies[qri];
    Dout(dc::notice, "Constructing priorities_per_family for " << reply << " when priorities_per_family[" << reply.get_queue_family() << "].size() = " <<
        priorities_per_family[reply.get_queue_family()].size());
    m_queue_replies[qri].set_start_index(priorities_per_family[reply.get_queue_family()].size());
    for (int q = 0; q < reply.number_of_queues(); ++q)
      priorities_per_family[reply.get_queue_family()].push_back(request.priority);
  }

  // Include each queue family with any of the requested features - in the pQueueCreateInfos of device_create_info.
  std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;
  for (auto iter = priorities_per_family.begin(); iter != priorities_per_family.end(); ++iter)
    queue_create_infos.push_back({
      .queueFamilyIndex = static_cast<uint32_t>(iter->first.get_value()),
      .queueCount = static_cast<uint32_t>(iter->second.size()),
      .pQueuePriorities = iter->second.data()
    });
  device_create_info.setQueueCreateInfos(queue_create_infos);

#ifdef CWDEBUG
  // Make a copy of the debug name in the device_create_info.
  set_debug_name(device_create_info.debug_name());
#endif

  Dout(dc::vulkan, "Calling m_vh_physical_device.createDevice(" << device_create_info << ")");
  m_device = m_vh_physical_device.createDeviceUnique(device_create_info);
  // For greater performance, immediately after creating a vulkan device, inform the extension loader.
  dispatch_loader.load(vulkan_instance, *m_device);

#ifdef CWDEBUG
  // Set the debug name of the device.
  // Note: when not using -DVULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1 this requires the dispatch_loader to be initialized (see the line above).
  vk::DebugUtilsObjectNameInfoEXT name_info{
    .objectType = vk::ObjectType::eDevice,
    .objectHandle = (uint64_t)static_cast<VkDevice>(*m_device),
    .pObjectName = debug_name().c_str()
  };
  Dout(dc::vulkan, "Setting debug name of device " << *m_device << " to " << debug::print_string(device_create_info.debug_name()));
  m_device->setDebugUtilsObjectNameEXT(name_info);
#endif
}

Queue LogicalDevice::acquire_queue(
    QueueFlags flags,
    task::SynchronousWindow::window_cookie_type window_cookie) const
{
  DoutEntering(dc::vulkan, "LogicalDevice::acquire_queue(" << flags << ", " << window_cookie << ")");
  // window_cookie is a bit mask and must represent a single window.
  ASSERT(utils::is_power_of_two(window_cookie));

  // Accumulate the combined queue flags.
  utils::Vector<QueueFlags, QueueRequestIndex> combined_queue_flags(m_queue_replies.size());
  // Run over all QueueReply's.
  QueueRequestIndex const qri_end{m_queue_replies.size()};
  for (QueueRequestIndex qri{0}; qri != qri_end; ++qri)
  {
    QueueReply const& reply = m_queue_replies[qri];
    if (!(reply.get_window_cookies() & window_cookie))          // Skip requests that are not to be used for this window.
      continue;
    combined_queue_flags[qri] |= reply.requested_queue_flags();
    if (!reply.combined_with().undefined())
      combined_queue_flags[reply.combined_with()] |= reply.requested_queue_flags();
  }

  // If there is only one QueueReply with flags support, return that.
  // In both of the following counts, combined reply's are NOT counted (because they are not an extra choice).
  int support_count = 0;        // The number of QueueReply's whose queue family supports flags (even if that is the same queue family).
  int request_count = 0;        // The number of QueueReply's whose request flags matches flags.
  QueueRequestIndex queue_request_index;        // The index into the vector of QueueReply's that matched (the last one if there are more).

  // Run over all QueueReply's.
  for (QueueRequestIndex qri{0}; qri != qri_end; ++qri)
  {
    QueueReply const& reply = m_queue_replies[qri];
    if (!reply.combined_with().undefined())                                     // Skip combined replies.
      continue;

    Dout(dc::notice, "Reply = " << reply << "; combined_queue_flags = " << combined_queue_flags[qri]);

    auto qfpi = reply.get_queue_family();
    if ((m_queue_families[qfpi].get_queue_flags() & flags) == flags)            // This queue family supports requested flags?
    {
      ++support_count;
      if ((combined_queue_flags[qri] & flags) == flags)                         // The corresponding QueueRequest requested the same flags?
      {
        queue_request_index = qri;                                              // We found a (the?) matching queue request/reply.
        ++request_count;
      }
    }
  }
  Dout(dc::notice, "Found " << support_count << " QueueReply's that support " << flags << " and " << request_count << " QueueRequest's that requested them.");

  // Deal with errors.
  if (support_count == 0)
    THROW_ALERT("While acquiring a queue with flags [FLAGS] for window cookie [COOKIE], no queue family was found at all that supports that.", AIArgs("[FLAGS]", flags)("[COOKIE]", window_cookie));
  if (request_count == 0)
  {
    // If flags has more than one bit set, return an empty handle indicating that the combination could not be found
    // and a new attempt with less bits should be tried.
    if (!utils::is_power_of_two(static_cast<QueueFlags::MaskType>(flags)))
      return {};
    THROW_ALERT("Trying to acquire a queue with flag [FLAGS] for window cookie [COOKIE] without having requested those flags.", AIArgs("[FLAGS]", flags)("[COOKIE]", window_cookie));
  }
  if (request_count > 1)
  {
    // Make sure there is only one solution per window for any combination of requested flags.
    THROW_ALERT("While acquiring a queue with flags [FLAGS] for window cookie [COOKIE], more than one solution was found.", AIArgs("[FLAGS]", flags)("[COOKIE]", window_cookie));
  }

  // Reserve a queue index from the pool of total queues.
  int next_queue_index = m_queue_replies[queue_request_index].acquire_queue();
  if (next_queue_index == -1)
  {
    THROW_ALERT("Trying to acquire a queue with flags [FLAGS], but we have run out; there were only [N] such queues.",
        AIArgs("[FLAGS]", flags)("[N]", m_queue_replies[queue_request_index].number_of_queues()));
  }

  // Allocate the queue and return the handle.
  QueueFamilyPropertiesIndex queue_family_properties_index = m_queue_replies[queue_request_index].get_queue_family();;
  Dout(dc::vulkan, "Calling vk::Device::getQueue(" << queue_family_properties_index.get_value() << ", " << next_queue_index << ")");
  return { m_device->getQueue(queue_family_properties_index.get_value(), next_queue_index), queue_family_properties_index };
}

vk::UniqueRenderPass LogicalDevice::create_render_pass(
    rendergraph::RenderPass const& render_graph_pass
    COMMA_CWDEBUG_ONLY(AmbifixOwner const& debug_name)) const
{
  auto const& attachment_descriptions = render_graph_pass.attachment_descriptions();
  auto const& subpass_descriptions = render_graph_pass.subpass_descriptions();

  std::vector<vk::SubpassDependency> dependencies = {
    // FIXME: For now leave dependencies away... the automatically generated ones should suffice - no?
#if 0
    {
      .srcSubpass = VK_SUBPASS_EXTERNAL,
      .dstSubpass = 0,
      .srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
      .dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
      .srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
      .dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
      .dependencyFlags = vk::DependencyFlagBits::eByRegion
    },
    {
      .srcSubpass = 0,
      .dstSubpass = VK_SUBPASS_EXTERNAL,
      .srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
      .dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
      .srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
      .dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
      .dependencyFlags = vk::DependencyFlagBits::eByRegion
    }
#endif
  };

  vk::RenderPassCreateInfo render_pass_create_info{
    .attachmentCount = static_cast<uint32_t>(attachment_descriptions.size()),
    .pAttachments = attachment_descriptions.data(),
    .subpassCount = static_cast<uint32_t>(subpass_descriptions.size()),
    .pSubpasses = subpass_descriptions.data(),
    .dependencyCount = static_cast<uint32_t>(dependencies.size()),
    .pDependencies = dependencies.data()
  };

  vk::UniqueRenderPass render_pass = m_device->createRenderPassUnique(render_pass_create_info);
  DebugSetName(render_pass, debug_name);
  return render_pass;
}

vk::UniqueImage LogicalDevice::create_image(
    vk::Extent2D extent,
    vulkan::ImageKind const& image_kind
    COMMA_CWDEBUG_ONLY(AmbifixOwner const& debug_name)) const
{
  DoutEntering(dc::vulkan, "LogicalDevice::create_image(" << extent << ", " << image_kind << ")");
  vk::UniqueImage image = m_device->createImageUnique(image_kind(extent));
  DebugSetName(image, debug_name);
  return image;
}

vk::UniqueImageView LogicalDevice::create_image_view(
    vk::Image vh_image, ImageViewKind const& image_view_kind
    COMMA_CWDEBUG_ONLY(AmbifixOwner const& debug_name)) const
{
  DoutEntering(dc::vulkan, "LogicalDevice::create_image_view(" << vh_image << ", " << image_view_kind << ")");
  vk::UniqueImageView image_view = m_device->createImageViewUnique(image_view_kind(vh_image));
  DebugSetName(image_view, debug_name);
  return image_view;
}

ImageParameters LogicalDevice::create_image(
    uint32_t width,
    uint32_t height,
    vulkan::ImageViewKind const& image_view_kind,
    vk::MemoryPropertyFlagBits property
    COMMA_CWDEBUG_ONLY(AmbifixOwner const& ambifix)) const
{
  vk::UniqueImage tmp_image = create_image({ width, height }, image_view_kind.image_kind()
      COMMA_CWDEBUG_ONLY(ambifix(".m_image")));

  vk::UniqueDeviceMemory tmp_memory = allocate_image_memory(*tmp_image, property
      COMMA_CWDEBUG_ONLY(ambifix(".m_memory")));

  m_device->bindImageMemory(*tmp_image, *tmp_memory, vk::DeviceSize(0));

  vk::UniqueImageView tmp_view = create_image_view(*tmp_image, image_view_kind
      COMMA_CWDEBUG_ONLY(ambifix(".m_image_view")));

  return {
   .m_image = std::move(tmp_image),
   .m_image_view = std::move(tmp_view),
   .m_memory = std::move(tmp_memory)
  };
}

vk::UniqueShaderModule LogicalDevice::create_shader_module(
    uint32_t const* spirv_code, size_t spirv_size
    COMMA_CWDEBUG_ONLY(AmbifixOwner const& debug_name)) const
{
  vk::ShaderModuleCreateInfo shader_module_create_info{
    .codeSize = spirv_size,     // Size is in bytes.
    .pCode = spirv_code
  };

  vk::UniqueShaderModule shader_module = m_device->createShaderModuleUnique(shader_module_create_info);
  DebugSetName(shader_module, debug_name);
  return shader_module;
}

vk::UniqueSampler LogicalDevice::create_sampler(
    SamplerKind const& sampler_kind
    COMMA_CWDEBUG_ONLY(AmbifixOwner const& debug_name)) const
{
  DoutEntering(dc::vulkan, "LogicalDevice::create_sampler(" << sampler_kind << ")");

  vk::UniqueSampler sampler = m_device->createSamplerUnique(sampler_kind());
  DebugSetName(sampler, debug_name);
  return sampler;
}

vk::UniqueDeviceMemory LogicalDevice::allocate_image_memory(
    vk::Image image,
    vk::MemoryPropertyFlagBits property
    COMMA_CWDEBUG_ONLY(AmbifixOwner const& debug_name)) const
{
  vk::MemoryRequirements image_memory_requirements = m_device->getImageMemoryRequirements(image);
  vk::PhysicalDeviceMemoryProperties memory_properties = m_vh_physical_device.getMemoryProperties();

  for (uint32_t i = 0; i < memory_properties.memoryTypeCount; ++i)
  {
    if ((image_memory_requirements.memoryTypeBits & (1 << i)) &&
      ((memory_properties.memoryTypes[i].propertyFlags & property) == property))
    {
      try
      {
        vk::UniqueDeviceMemory memory = m_device->allocateMemoryUnique({.allocationSize = image_memory_requirements.size, .memoryTypeIndex = i});
        DebugSetName(memory, debug_name);
        return memory;
      }
      catch (...)
      {
        // Iterate over all supported memory types; only if none of them could be used, throw exception.
      }
    }
  }
  throw std::runtime_error("Could not allocate a memory for an image!");
}

vk::UniqueFramebuffer LogicalDevice::create_imageless_framebuffer(
    RenderPass const& render_pass,
    vk::Extent2D extent, uint32_t layers
    COMMA_CWDEBUG_ONLY(AmbifixOwner const& debug_name)) const
{
  utils::Vector<vk::FramebufferAttachmentImageInfo, vulkan::rendergraph::pAttachmentsIndex> framebuffer_attachment_image_infos =
    render_pass.get_framebuffer_attachment_image_infos(extent);

  vk::StructureChain<vk::FramebufferCreateInfo, vk::FramebufferAttachmentsCreateInfo> framebuffer_create_info_chain(
    {
      .flags = vk::FramebufferCreateFlagBits::eImageless,
      .renderPass = render_pass.vh_render_pass(),
      .attachmentCount = static_cast<uint32_t>(framebuffer_attachment_image_infos.size()),
      .width = extent.width,
      .height = extent.height,
      .layers = layers
    },
    {
      .attachmentImageInfoCount = static_cast<uint32_t>(framebuffer_attachment_image_infos.size()),
      .pAttachmentImageInfos = framebuffer_attachment_image_infos.data()
    }
  );

  vk::UniqueFramebuffer framebuffer = m_device->createFramebufferUnique(framebuffer_create_info_chain.get<vk::FramebufferCreateInfo>());
  DebugSetName(framebuffer, debug_name);
  return framebuffer;
}

vk::UniqueDescriptorPool LogicalDevice::create_descriptor_pool(
    std::vector<vk::DescriptorPoolSize> const& pool_sizes,
    uint32_t max_sets
    COMMA_CWDEBUG_ONLY(AmbifixOwner const& debug_name)) const
{
  vk::DescriptorPoolCreateInfo descriptor_pool_create_info{
    .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
    .maxSets = max_sets,
    .poolSizeCount = static_cast<uint32_t>(pool_sizes.size()),
    .pPoolSizes = pool_sizes.data()
  };
  vk::UniqueDescriptorPool descriptor_pool = m_device->createDescriptorPoolUnique(descriptor_pool_create_info);
  DebugSetName(descriptor_pool, debug_name);
  return descriptor_pool;
}

vk::UniqueDescriptorSetLayout LogicalDevice::create_descriptor_set_layout(
    std::vector<vk::DescriptorSetLayoutBinding> const& layout_bindings
    COMMA_CWDEBUG_ONLY(AmbifixOwner const& debug_name)) const
{
  vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_create_info{
    .bindingCount = static_cast<uint32_t>(layout_bindings.size()),
    .pBindings = layout_bindings.data()
  };
  vk::UniqueDescriptorSetLayout set_layout = m_device->createDescriptorSetLayoutUnique(descriptor_set_layout_create_info);
  DebugSetName(set_layout, debug_name);
  return set_layout;
}

std::vector<vk::UniqueDescriptorSet> LogicalDevice::allocate_descriptor_sets(
    std::vector<vk::DescriptorSetLayout> const& descriptor_set_layout,
    vk::DescriptorPool descriptor_pool
    COMMA_CWDEBUG_ONLY(AmbifixOwner const& debug_name)) const
{
  vk::DescriptorSetAllocateInfo descriptor_set_allocate_info{
    .descriptorPool = descriptor_pool,
    .descriptorSetCount = static_cast<uint32_t>(descriptor_set_layout.size()),
    .pSetLayouts = descriptor_set_layout.data()
  };
  std::vector<vk::UniqueDescriptorSet> descriptor_sets = m_device->allocateDescriptorSetsUnique(descriptor_set_allocate_info);
#ifdef CWDEBUG
  for (int i = 0; i < descriptor_sets.size(); ++i)
    DebugSetName(descriptor_sets[i], debug_name("[" + to_string(i) + "]"));
#endif
  return descriptor_sets;
}

void LogicalDevice::allocate_command_buffers(
    vk::CommandPool const& pool,
    vk::CommandBufferLevel level,
    uint32_t count,
    vk::CommandBuffer* command_buffers_out
    COMMA_CWDEBUG_ONLY(AmbifixOwner const& debug_name, bool is_array)) const
{
  DoutEntering(dc::vulkan, "LogicalDevice::allocate_command_buffers(" << pool << ", " << level << ", " << count << ", " << command_buffers_out << ") [" << this << "]");

  vk::CommandBufferAllocateInfo command_buffer_allocate_info{
    .commandPool = pool,
    .level = level,
    .commandBufferCount = count
  };

  vk::Result result = m_device->allocateCommandBuffers(&command_buffer_allocate_info, command_buffers_out);
  if (AI_UNLIKELY(result != vk::Result::eSuccess))
#ifdef CWDEBUG
    THROW_ALERTC(result, "[DEVICE]->allocateCommandBuffers", AIArgs("[DEVICE]", this->debug_name()));
#else
    THROW_ALERTC(result, "LogicalDevice::allocateCommandBuffers");
#endif

#ifdef CWDEBUG
  if (!is_array)
    DebugSetName(*command_buffers_out, debug_name);
  else
    for (int i = 0; i < count; ++i)
      DebugSetName(command_buffers_out[i], debug_name("[" + std::to_string(i) + "]"));
#endif
}

DescriptorSetParameters LogicalDevice::create_descriptor_resources(
    std::vector<vk::DescriptorSetLayoutBinding> const& layout_bindings,
    std::vector<vk::DescriptorPoolSize> const& pool_sizes
    COMMA_CWDEBUG_ONLY(AmbifixOwner const& ambifix)) const
{
  vk::UniqueDescriptorSetLayout descriptor_set_layout = create_descriptor_set_layout(layout_bindings
      COMMA_CWDEBUG_ONLY(ambifix(".m_layout")));

  vk::UniqueDescriptorPool descriptor_pool = create_descriptor_pool(pool_sizes, 1
      COMMA_CWDEBUG_ONLY(ambifix(".m_pool")));

  std::vector<vk::UniqueDescriptorSet> descriptor_sets = allocate_descriptor_sets({ *descriptor_set_layout }, *descriptor_pool
      COMMA_CWDEBUG_ONLY(ambifix(".m_handle")));

  return {
   .m_pool = std::move(descriptor_pool),
   .m_handle = std::move(descriptor_sets[0]),
   .m_layout = std::move(descriptor_set_layout)
  };
}

void LogicalDevice::update_descriptor_set(
    vk::DescriptorSet descriptor_set,
    vk::DescriptorType descriptor_type,
    uint32_t binding,
    uint32_t array_element,
    std::vector<vk::DescriptorImageInfo> const& image_infos,
    std::vector<vk::DescriptorBufferInfo> const& buffer_infos,
    std::vector<vk::BufferView> const& buffer_views) const
{
  vk::WriteDescriptorSet descriptor_writes{
    .dstSet = descriptor_set,
    .dstBinding = binding,
    .dstArrayElement = array_element,
    .descriptorCount = 1,
    .descriptorType = descriptor_type,
    .pImageInfo = image_infos.size() ? image_infos.data() : nullptr,
    .pBufferInfo = buffer_infos.size() ? buffer_infos.data() : nullptr,
    .pTexelBufferView = buffer_views.size() ? buffer_views.data() : nullptr
  };

  m_device->updateDescriptorSets(1, &descriptor_writes, 0, nullptr);
}

vk::UniquePipelineLayout LogicalDevice::create_pipeline_layout(
    std::vector<vk::DescriptorSetLayout> const& descriptor_set_layouts,
    std::vector<vk::PushConstantRange> const& push_constant_ranges
    COMMA_CWDEBUG_ONLY(AmbifixOwner const& debug_name)) const
{
  vk::PipelineLayoutCreateInfo layout_create_info{
    .flags = {},
    .setLayoutCount = static_cast<uint32_t>(descriptor_set_layouts.size()),
    .pSetLayouts = descriptor_set_layouts.data(),
    .pushConstantRangeCount = static_cast<uint32_t>(push_constant_ranges.size()),
    .pPushConstantRanges = push_constant_ranges.data()
  };

  vk::UniquePipelineLayout pipline_layout = m_device->createPipelineLayoutUnique(layout_create_info);
  DebugSetName(pipline_layout, debug_name);
  return pipline_layout;
}

vk::UniqueBuffer LogicalDevice::create_buffer(
    uint32_t size,
    vk::BufferUsageFlags usage
    COMMA_CWDEBUG_ONLY(AmbifixOwner const& debug_name)) const
{
  vk::BufferCreateInfo buffer_create_info{
    .flags = vk::BufferCreateFlags(0),
    .size = size,
    .usage = usage
  };
  vk::UniqueBuffer buffer = m_device->createBufferUnique(buffer_create_info);
  DebugSetName(buffer, debug_name);
  return buffer;
}

vk::UniqueDeviceMemory LogicalDevice::allocate_buffer_memory(
    vk::Buffer buffer,
    vk::MemoryPropertyFlagBits property
    COMMA_CWDEBUG_ONLY(AmbifixOwner const& debug_name)) const
{
  vk::MemoryRequirements buffer_memory_requirements = m_device->getBufferMemoryRequirements(buffer);
  vk::PhysicalDeviceMemoryProperties memory_properties = m_vh_physical_device.getMemoryProperties();

  for (uint32_t i = 0; i < memory_properties.memoryTypeCount; ++i)
  {
    if ((buffer_memory_requirements.memoryTypeBits & (1 << i)) &&
        ((memory_properties.memoryTypes[i].propertyFlags & property) == property) ) {
      try
      {
        vk::UniqueDeviceMemory memory = m_device->allocateMemoryUnique({ .allocationSize = buffer_memory_requirements.size, .memoryTypeIndex = i });
        DebugSetName(memory, debug_name);
        return memory;
      }
      catch( ... )
      {
        // Iterate over all supported memory types; only if none of them could be used, throw exception
      }
    }
  }

  throw std::runtime_error( "Could not allocate a memory for a buffer!" );
}

BufferParameters LogicalDevice::create_buffer(
    uint32_t size,
    vk::BufferUsageFlags usage,
    vk::MemoryPropertyFlagBits memory_property
    COMMA_CWDEBUG_ONLY(AmbifixOwner const& ambifix)) const
{
  if (!(memory_property & vk::MemoryPropertyFlagBits::eHostCoherent))
  {
    // Allocate a multiple of m_non_coherent_atom_size bytes.
    size = ((size - 1) / m_non_coherent_atom_size + 1) * m_non_coherent_atom_size;
  }

  vk::UniqueBuffer buffer = create_buffer(size, usage
      COMMA_CWDEBUG_ONLY(ambifix(".m_buffer")));

  vk::UniqueDeviceMemory buffer_memory = allocate_buffer_memory(*buffer, memory_property
      COMMA_CWDEBUG_ONLY(ambifix(".m_memory")));

  m_device->bindBufferMemory(*buffer, *buffer_memory, vk::DeviceSize(0));

  return {
    .m_buffer = std::move(buffer),
    .m_memory = std::move(buffer_memory),
    .m_size = size
  };
}

vk::UniqueSwapchainKHR LogicalDevice::create_swapchain(
    vk::Extent2D extent,
    uint32_t min_image_count,
    PresentationSurface const& presentation_surface,
    SwapchainKind const& swapchain_kind,
    vk::SwapchainKHR vh_old_swapchain
    COMMA_CWDEBUG_ONLY(AmbifixOwner const& debug_name)) const
{
  Dout(dc::vulkan, "LogicalDevice::create_swapchain(" << extent << ", " << swapchain_kind << ", " << vh_old_swapchain << ")");
  vk::UniqueSwapchainKHR swapchain = m_device->createSwapchainKHRUnique(swapchain_kind(extent, min_image_count, presentation_surface, vh_old_swapchain));
  DebugSetName(swapchain, debug_name);
  return swapchain;
}

vk::UniquePipeline LogicalDevice::create_graphics_pipeline(
    vk::PipelineCache vh_pipeline_cache,
    vk::GraphicsPipelineCreateInfo const& graphics_pipeline_create_info
    COMMA_CWDEBUG_ONLY(AmbifixOwner const& debug_name)) const
{
  DoutEntering(dc::vulkan, "LogicalDevice::create_graphics_pipeline(" << vh_pipeline_cache << ", " << graphics_pipeline_create_info << ")");
  vk::UniquePipeline pipeline = m_device->createGraphicsPipelineUnique(vh_pipeline_cache, graphics_pipeline_create_info).value;
  DebugSetName(pipeline, debug_name);
  return pipeline;
}

Swapchain::images_type LogicalDevice::get_swapchain_images(
    task::SynchronousWindow const* owning_window,
    vk::SwapchainKHR vh_swapchain
    COMMA_CWDEBUG_ONLY(AmbifixOwner const& ambifix)) const
{
  DoutEntering(dc::vulkan, "LogicalDevice::get_swapchain_images(" << vh_swapchain << ")");
  Swapchain::images_type swapchain_images;
  swapchain_images = m_device->getSwapchainImagesKHR(vh_swapchain);

  ResourceState const new_image_resource_state;
  ResourceState const initial_present_resource_state = {
    .pipeline_stage_mask        = vk::PipelineStageFlagBits::eBottomOfPipe,
    .access_mask                = vk::AccessFlagBits::eMemoryRead,
    .layout                     = vk::ImageLayout::ePresentSrcKHR
  };

  for (SwapchainIndex i = swapchain_images.ibegin(); i != swapchain_images.iend(); ++i)
  {
    DebugSetName(swapchain_images[i], ambifix("[" + to_string(i) + "]"));
    // Pre-transition all swapchain images away from an undefined layout.
    owning_window->set_image_memory_barrier(
      new_image_resource_state,
      initial_present_resource_state,
      swapchain_images[i],
      Swapchain::s_default_subresource_range);
  }

  return swapchain_images;
}

void* LogicalDevice::map_memory(vk::DeviceMemory vh_memory, vk::DeviceSize offset, vk::DeviceSize size) const
{
  DoutEntering(dc::vulkan|dc::vkframe, "LogicalDevice::map_memory(" << vh_memory << ", " << offset << ", " << size << ")");
  return m_device->mapMemory(vh_memory, offset, size);
}

void LogicalDevice::flush_mapped_memory_ranges(vk::ArrayProxy<vk::MappedMemoryRange const> const& mapped_memory_ranges) const
{
  DoutEntering(dc::vulkan|dc::vkframe, "LogicalDevice::flush_mapped_memory_ranges(" << mapped_memory_ranges << ")");
  m_device->flushMappedMemoryRanges(mapped_memory_ranges);
}

void LogicalDevice::unmap_memory(vk::DeviceMemory vh_memory) const
{
  DoutEntering(dc::vulkan|dc::vkframe, "LogicalDevice::unmap_memory(" << vh_memory << ")");
  m_device->unmapMemory(vh_memory);
}

#ifdef CWDEBUG
void LogicalDevice::print_members(std::ostream& os, char const* prefix) const
{
  os << prefix <<
      "m_vh_physical_device:"   << m_vh_physical_device <<
    ", m_device:\""             << debug_name() << "\" [" << *m_device << "]"
    ", m_queue_replies: "       << m_queue_replies;
}
#endif

} // namespace vulkan

namespace task {

LogicalDevice::LogicalDevice(vulkan::Application* application COMMA_CWDEBUG_ONLY(bool debug)) :
  AIStatefulTask(CWDEBUG_ONLY(debug)), m_application(application)
{
}

LogicalDevice::~LogicalDevice()
{
}

void LogicalDevice::set_root_window(boost::intrusive_ptr<task::SynchronousWindow const>&& root_window)
{
  // The const_pointer_cast is OK because m_root_window is only used during initialization - aka, synchronous.
  m_root_window = boost::const_pointer_cast<task::SynchronousWindow>(std::move(root_window));
}

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
      // Wait until m_root_window was created (so we can get its Surface).
      m_root_window->m_window_created_event.register_task(this, window_available_condition);
      set_state(LogicalDevice_create);
      wait(window_available_condition);
      break;
    case LogicalDevice_create:
      // Create a logical device that supports presentation on m_root_window and add it to the application.
      m_index = m_application->create_device(std::move(m_logical_device), m_root_window.get());
      // Store a cached value of the vulkan::LogicalDevice* in this window.
      m_root_window->cache_logical_device();
      // Notify child windows that the logical device index is available.
      m_logical_device_index_available_event.trigger();
      // Allow deletion of this window now that we're done with this pointer.
      m_root_window.reset();
      // This task is done.
      set_state(LogicalDevice_done);
      [[fallthrough]];
    case LogicalDevice_done:
      finish();
      break;
  }
}

} // namespace task
