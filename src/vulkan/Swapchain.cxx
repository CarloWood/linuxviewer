#include "sys.h"
#include "Defaults.h"
#include "Swapchain.h"
#include "PresentationSurface.h"
#include "LogicalDevice.h"
#include "FrameResourcesData.h"
#include "DelayedDestroyer.h"
#include "debug.h"
#include "vk_utils/print_flags.h"
#ifdef CWDEBUG
#include "debug/debug_ostream_operators.h"
#include "debug/vulkan_print_on.h"
#endif

namespace {

vk::Extent2D choose_extent(vk::SurfaceCapabilitiesKHR const& surface_capabilities, vk::Extent2D actual_extent)
{
  // The value {-1, -1} is special.
  if (surface_capabilities.currentExtent.width == std::numeric_limits<uint32_t>::max())
    return { std::clamp(actual_extent.width, surface_capabilities.minImageExtent.width, surface_capabilities.maxImageExtent.width),
             std::clamp(actual_extent.height, surface_capabilities.minImageExtent.height, surface_capabilities.maxImageExtent.height) };

  // Most of the cases we define size of the swapchain images equal to current window's size.
  return surface_capabilities.currentExtent;
}

vk::SurfaceFormatKHR choose_surface_format(std::vector<vk::SurfaceFormatKHR> const& available_formats)
{
  DoutEntering(dc::vulkan, "choose_surface_format(" << available_formats << ")");

  static std::array<vk::SurfaceFormatKHR, 3> desired_formats = {{
    { vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear },
    { vk::Format::eR8G8B8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear },
    { vk::Format::eA8B8G8R8SrgbPack32, vk::ColorSpaceKHR::eSrgbNonlinear }
  }};

  // If the list contains only one entry with undefined format it means that there are no preferred surface formats and any can be chosen.
  if (available_formats.size() == 1 && available_formats[0].format == vk::Format::eUndefined)
    return desired_formats[0];

  for (auto const& desired_format : desired_formats)
    for (auto const& available_format : available_formats)
      if (available_format == desired_format)
      {
        Dout(dc::vulkan, "Picked desired format: " << desired_format);
        return available_format;
      }

  // This should have thrown an exception before we got here.
  ASSERT(!available_formats.empty());

  Dout(dc::vulkan, "Returning format: " << available_formats[0]);
  return available_formats[0];
}

vk::ImageUsageFlags choose_usage_flags(vk::SurfaceCapabilitiesKHR const& surface_capabilities, vk::ImageUsageFlags const selected_usage)
{
  // Color attachment flag must always be supported.
  // We can define other usage flags but we always need to check if they are supported.
  vk::ImageUsageFlags available_flags = surface_capabilities.supportedUsageFlags & selected_usage;

  if (!available_flags || ((selected_usage & vk::ImageUsageFlagBits::eColorAttachment) && !(available_flags & vk::ImageUsageFlagBits::eColorAttachment)))
    THROW_ALERT("Unsupported swapchain image usage flags requested ([REQUESTED]). Supported swapchain image usages include [SUPPORTED].",
        AIArgs("[REQUESTED]", selected_usage)("[SUPPORTED]", surface_capabilities.supportedUsageFlags));

  return available_flags;
}

vk::PresentModeKHR choose_present_mode(std::vector<vk::PresentModeKHR> const& available_present_modes, vk::PresentModeKHR selected_present_mode)
{
  auto have_present_mode = [&](vk::PresentModeKHR requested_present_mode){
    for (auto const& available_present_mode : available_present_modes)
      if (available_present_mode == requested_present_mode)
      {
        Dout(dc::vulkan, "Present mode: " << requested_present_mode);
        return true;
      }
    return false;
  };

  if (have_present_mode(selected_present_mode))
    return selected_present_mode;

  Dout(dc::vulkan|dc::warning, "Requested present mode " << selected_present_mode << " not available!");

  if (have_present_mode(vk::PresentModeKHR::eImmediate))
    return vk::PresentModeKHR::eImmediate;

  if (have_present_mode(vk::PresentModeKHR::eMailbox))
    return vk::PresentModeKHR::eMailbox;

  if (have_present_mode(vk::PresentModeKHR::eFifoRelaxed))
    return vk::PresentModeKHR::eFifoRelaxed;

  if (!have_present_mode(vk::PresentModeKHR::eFifo))
    THROW_ALERT("FIFO present mode is not supported by the swap chain!");

  return vk::PresentModeKHR::eFifo;
}

uint32_t get_number_of_images(vk::SurfaceCapabilitiesKHR const& surface_capabilities, uint32_t selected_image_count)
{
  uint32_t max_image_count = surface_capabilities.maxImageCount > 0 ? surface_capabilities.maxImageCount : std::numeric_limits<uint32_t>::max();
  uint32_t image_count = std::clamp(selected_image_count, surface_capabilities.minImageCount, max_image_count);

  return image_count;
}

vk::SurfaceTransformFlagBitsKHR get_transform(vk::SurfaceCapabilitiesKHR const& surface_capabilities)
{
  // Sometimes images must be transformed before they are presented (i.e. due to device's orienation being other than default orientation).
  // If the specified transform is other than current transform, presentation engine will transform image during presentation operation;
  // this operation may hit performance on some platforms.
  // Here we don't want any transformations to occur so if the identity transform is supported use it otherwise just use the same
  // transform as current transform.
  if ((surface_capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity))
    return vk::SurfaceTransformFlagBitsKHR::eIdentity;

  return surface_capabilities.currentTransform;
}

} // namespace

namespace vulkan {

void Swapchain::prepare(task::SynchronousWindow* owning_window, vk::ImageUsageFlags const selected_usage, vk::PresentModeKHR const selected_present_mode
    COMMA_CWDEBUG_ONLY(vulkan::AmbifixOwner const& ambifix))
{
  DoutEntering(dc::vulkan, "Swapchain::prepare(" << owning_window << ", " << ", " << selected_usage << ", " << selected_present_mode << ")");

  vk::PhysicalDevice vh_physical_device = owning_window->logical_device().vh_physical_device();
  PresentationSurface const& presentation_surface = owning_window->presentation_surface();

  // Query supported surface details.
  vk::SurfaceCapabilitiesKHR        surface_capabilities =    vh_physical_device.getSurfaceCapabilitiesKHR(presentation_surface.vh_surface());
  std::vector<vk::SurfaceFormatKHR> surface_formats =         vh_physical_device.getSurfaceFormatsKHR(presentation_surface.vh_surface());
  std::vector<vk::PresentModeKHR>   available_present_modes = vh_physical_device.getSurfacePresentModesKHR(presentation_surface.vh_surface());

  Dout(dc::vulkan, "Surface capabilities: " << surface_capabilities);
  Dout(dc::vulkan, "Supported surface formats: " << surface_formats);
  Dout(dc::vulkan, "Available present modes: " << available_present_modes);

  vk::Extent2D                    desired_extent = choose_extent(surface_capabilities, owning_window->get_extent());
  vk::SurfaceFormatKHR            desired_image_format = choose_surface_format(surface_formats);
  vk::ImageUsageFlags             desired_image_usage_flags = choose_usage_flags(surface_capabilities, selected_usage);
  vk::PresentModeKHR              desired_present_mode = choose_present_mode(available_present_modes, selected_present_mode);
  uint32_t const                  desired_image_count = get_number_of_images(surface_capabilities, 2);
  vk::SurfaceTransformFlagBitsKHR desired_transform = get_transform(surface_capabilities);

  Dout(dc::vulkan, "Requesting " << desired_image_count << " swap chain images (with extent " << desired_extent << ")");
  Dout(dc::vulkan, "Chosen format: " << desired_image_format);
  Dout(dc::vulkan, "Chosen usage: " << desired_image_usage_flags);
  Dout(dc::vulkan, "Chosen present mode: " << desired_present_mode);
  Dout(dc::vulkan, "Used transform: " << desired_transform);

  vk::SharingMode                 desired_sharing_mode = vk::SharingMode::eExclusive;
  uint32_t                        desired_queue_family_index_count = 0;
  uint32_t const*                 desired_queue_family_indices = nullptr;

  if (presentation_surface.uses_multiple_queue_families())
  {
    m_queue_family_indices = presentation_surface.queue_family_indices();
    desired_queue_family_index_count = m_queue_family_indices.size();
    desired_queue_family_indices = m_queue_family_indices.data();
    desired_sharing_mode = vk::SharingMode::eConcurrent;
  }

  // Note: set() must be called before set_image_kind() because of flags propagation.
  m_kind.set({},
    {
      .image_color_space = desired_image_format.colorSpace,
      .pre_transform = desired_transform,
      .present_mode = desired_present_mode
    }
  ).set_image_kind({}, {
      .format = desired_image_format.format,
      .usage = desired_image_usage_flags,
      .sharing_mode = desired_sharing_mode,
      .m_queue_family_index_count = desired_queue_family_index_count,
      .m_queue_family_indices = desired_queue_family_indices
  });

  // Perform the delayed initialization of m_presentation_attachment.
  m_presentation_attachment.emplace(utils::Badge<Swapchain>{}, owning_window, "swapchain", image_view_kind());
  Dout(dc::notice, "m_presentation_attachment->image_view_kind() = " << m_presentation_attachment->image_view_kind());

  m_min_image_count = desired_image_count;

  m_acquire_semaphore = owning_window->logical_device().create_semaphore(
        CWDEBUG_ONLY(ambifix(".m_acquire_semaphore")));

  // In case of re-use, cant_render_bit might be reset.
  owning_window->no_swapchain({});
}

void Swapchain::recreate(task::SynchronousWindow* owning_window, vk::Extent2D window_extent
    COMMA_CWDEBUG_ONLY(vulkan::AmbifixOwner const& ambifix))
{
  owning_window->no_swapchain({});

  if (window_extent.width == 0 || window_extent.height == 0)
  {
    // Current surface size is (0, 0) so we can't create a swapchain or render anything (cant_render_bit stays set!)
    // But we don't want to kill the application as this situation may occur i.e. when window gets minimized.
    return;
  }

  recreate_swapchain_images(owning_window, window_extent
      COMMA_CWDEBUG_ONLY(ambifix));

  owning_window->have_swapchain({});
}

void Swapchain::recreate_swapchain_images(task::SynchronousWindow* owning_window, vk::Extent2D surface_extent
    COMMA_CWDEBUG_ONLY(vulkan::AmbifixOwner const& ambifix))
{
  DoutEntering(dc::vulkan, "Swapchain::recreate_swapchain_images(" << owning_window << ", " << surface_extent << ")");

  LogicalDevice const& logical_device = owning_window->logical_device();
  PresentationSurface const& presentation_surface = owning_window->presentation_surface();

  // Keep a copy of the rendering_finished_semaphore's for number-of-swapchain-semaphores calls to acquire_image.
  int const frames_to_keep_rendering_finished_semaphore = m_resources.size();
  for (auto&& resources : m_resources)
    owning_window->m_delay_by_completed_draw_frames.add({}, resources.rescue_rendering_finished_semaphore(), frames_to_keep_rendering_finished_semaphore);

  // Delete the old images, views and semaphores.
  m_vhv_images.clear();
  m_resources.clear();

  vk::UniqueSwapchainKHR old_handle(std::move(m_swapchain));

  m_extent = surface_extent;
  m_swapchain = logical_device.create_swapchain(surface_extent, m_min_image_count, owning_window->presentation_surface(), m_kind, *old_handle
      COMMA_CWDEBUG_ONLY(ambifix(".m_swapchain")));
  m_vhv_images = logical_device.get_swapchain_images(owning_window, *m_swapchain
      COMMA_CWDEBUG_ONLY(ambifix(".m_vhv_images")));
  Dout(dc::vulkan, "Actual number of swap chain images: " << m_vhv_images.size());

  // Create the corresponding resources: image view and semaphores.
  for (SwapchainIndex i = m_vhv_images.ibegin(); i != m_vhv_images.iend(); ++i)
  {
    vk::UniqueImageView image_view = logical_device.create_image_view(m_vhv_images[i], image_view_kind()
        COMMA_CWDEBUG_ONLY(ambifix(".m_resources[" + to_string(i) + "].m_image_view")));
    vk::UniqueSemaphore image_available_semaphore = logical_device.create_semaphore(
        CWDEBUG_ONLY(ambifix(".m_resources[" + to_string(i) + "].m_vh_image_available_semaphore")));
    vk::UniqueSemaphore rendering_finished_semaphore = logical_device.create_semaphore(
        CWDEBUG_ONLY(ambifix(".m_resources[" + to_string(i) + "].m_rendering_finished_semaphore")));

    m_resources.emplace_back(std::move(image_view), std::move(image_available_semaphore), std::move(rendering_finished_semaphore));
  }
}

vk::RenderPass Swapchain::vh_render_pass() const
{
  return m_render_pass_output_sink->vh_render_pass();
}

vk::Framebuffer Swapchain::vh_framebuffer() const
{
  return m_render_pass_output_sink->vh_framebuffer();
}

} // namespace vulkan
