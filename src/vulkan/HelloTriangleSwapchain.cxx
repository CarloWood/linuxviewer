#include "sys.h"
#include "HelloTriangleSwapchain.h"
#include "debug.h"
#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>
#ifdef CWDEBUG
#include "debug_ostream_operators.h"
#endif

namespace vulkan {

// Local helper functions.
namespace {

uint32_t find_memory_type(uint32_t type_filter, vk::MemoryPropertyFlags properties, vk::PhysicalDevice vh_physical_device)
{
  vk::PhysicalDeviceMemoryProperties memory_properties;
  memory_properties = vh_physical_device.getMemoryProperties();
  for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++)
  {
    if ((type_filter & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties) { return i; }
  }

  throw std::runtime_error("failed to find suitable memory type!");
}

void create_image_with_info(vk::ImageCreateInfo const& image_create_info, vk::MemoryPropertyFlags properties, Device const& device,
    // Output variables:
    vk::Image& vh_image, vk::DeviceMemory& vh_image_memory)
{
  vh_image = device->createImage(image_create_info);

  vk::MemoryRequirements memory_requirements;
  device->getImageMemoryRequirements(vh_image, &memory_requirements);

  vk::MemoryAllocateInfo alloc_create_info{};
  alloc_create_info.allocationSize  = memory_requirements.size;
  alloc_create_info.memoryTypeIndex = find_memory_type(memory_requirements.memoryTypeBits, properties, device.vh_physical_device());

  vh_image_memory = device->allocateMemory(alloc_create_info);

  device->bindImageMemory(vh_image, vh_image_memory, 0);
}

vk::Extent2D choose_swap_extent(vk::SurfaceCapabilitiesKHR const& surface_capabilities, vk::Extent2D actual_extent)
{
  if (surface_capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
  {
    return surface_capabilities.currentExtent;
  }
  else
  {
    actual_extent.width  = std::max(surface_capabilities.minImageExtent.width, std::min(surface_capabilities.maxImageExtent.width, actual_extent.width));
    actual_extent.height = std::max(surface_capabilities.minImageExtent.height, std::min(surface_capabilities.maxImageExtent.height, actual_extent.height));

    return actual_extent;
  }
}

vk::SurfaceFormatKHR choose_swap_surface_format(std::vector<vk::SurfaceFormatKHR> const& surface_formats)
{
  // FIXME: shouldn't we prefer B8G8R8A8Srgb ?

  // If the list contains only one entry with undefined format
  // it means that there are no preferred surface formats and any can be chosen
  if (surface_formats.size() == 1 && surface_formats[0].format == vk::Format::eUndefined)
    return { vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear };

  for (auto const& surface_format : surface_formats)
  {
    if (surface_format.format == vk::Format::eB8G8R8A8Unorm && surface_format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
      return surface_format;
  }

  // This should have thrown an exception before we got here.
  ASSERT(!surface_formats.empty());

  return surface_formats[0];
}

vk::PresentModeKHR choose_swap_present_mode(std::vector<vk::PresentModeKHR> const& available_present_modes)
{
  // for (auto const& available_present_mode : available_present_modes)
  // {
  //  if (available_present_mode == vk::PresentModeKHR::eMailbox)
  //  {
  //    Dout(dc::vulkan, "Present mode: Mailbox");
  //    return available_present_mode;
  //  }
  // }

  // for (auto const& available_present_mode : available_present_modes)
  // {
  //   if (available_present_mode == vk::PresentModeKHR::eImmediate) {
  //     Dout(dc::vulkan, "Present mode: Immediate");
  //     return available_present_mode;
  //   }
  // }

  Dout(dc::vulkan, "Present mode: V-Sync");
  return vk::PresentModeKHR::eFifo;
}

vk::Format find_supported_format(std::vector<vk::Format> const& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features, vk::PhysicalDevice vh_physical_device)
{
  for (vk::Format format : candidates)
  {
    vk::FormatProperties properties = vh_physical_device.getFormatProperties(format);
    if ((tiling == vk::ImageTiling::eLinear  && (properties.linearTilingFeatures & features) == features) ||
        (tiling == vk::ImageTiling::eOptimal && (properties.optimalTilingFeatures & features) == features))
      return format;
  }
  throw std::runtime_error("failed to find supported format!");
}

} // namespace

// Constructor.
HelloTriangleSwapchain::HelloTriangleSwapchain(Device const& device) : m_device{device} { }

// And initialization.
void HelloTriangleSwapchain::setup(vk::Extent2D extent, Queue graphics_queue, Queue present_queue, vk::SurfaceKHR vh_surface)
{
  DoutEntering(dc::vulkan, "HelloTriangleSwapchain::setup(" << extent << ", " << graphics_queue << ", " << present_queue << ", " << vh_surface << ")");
  m_window_extent = extent;
  m_vh_graphics_queue = graphics_queue.vh_queue();
  m_vh_present_queue = present_queue.vh_queue();
  createSwapchain(vh_surface, graphics_queue, present_queue);
  createImageViews();
  createRenderPass();
  createDepthResources();
  createFramebuffers();
  createSyncObjects();
}

// Destructor.
HelloTriangleSwapchain::~HelloTriangleSwapchain()
{
  for (auto image_view : m_vhv_swapchain_image_views) { m_device->destroyImageView(image_view); }
  m_vhv_swapchain_image_views.clear();

  if (m_vh_swapchain)
  {
    m_device->destroySwapchainKHR(m_vh_swapchain);
    m_vh_swapchain = nullptr;
  }

  for (SwapchainIndex i(0); i != m_swapchain_end; ++i)
  {
    m_device->destroyImageView(m_vhv_depth_image_views[i]);
    m_device->destroyImage(m_vhv_depth_images[i]);
    m_device->freeMemory(m_vhv_depth_image_memorys[i]);
  }

  for (auto framebuffer : m_vhv_swapchain_framebuffers) { m_device->destroyFramebuffer(framebuffer); }

  m_device->destroyRenderPass(m_vh_render_pass);

  // Cleanup synchronization objects.
  for (SwapchainIndex i(0); i != m_vhv_swapchain_images.iend(); ++i)
  {
    m_device->destroySemaphore(m_vhv_render_finished_semaphores[i]);
    m_device->destroySemaphore(m_vhv_image_available_semaphores[i]);
    m_device->destroyFence(m_vhv_in_flight_fences[i]);
  }
}

//=============================================================================
// Functions called by setup
//

void HelloTriangleSwapchain::createSwapchain(vk::SurfaceKHR vh_surface, Queue graphics_queue, Queue present_queue)
{
  DoutEntering(dc::vulkan, "HelloTriangleSwapchain::createSwapchain(" << vh_surface << ", " << graphics_queue << ", " << present_queue << ")");

  // Query supported surface details.
  vk::PhysicalDevice vh_physical_device = m_device.vh_physical_device();
  vk::SurfaceCapabilitiesKHR surface_capabilities = vh_physical_device.getSurfaceCapabilitiesKHR(vh_surface);
  Dout(dc::vulkan, "Surface capabilities: " << surface_capabilities);
  std::vector<vk::SurfaceFormatKHR> surface_formats = vh_physical_device.getSurfaceFormatsKHR(vh_surface);
  Dout(dc::vulkan, "Supported surface formats: " << surface_formats);
  std::vector<vk::PresentModeKHR> available_present_modes = vh_physical_device.getSurfacePresentModesKHR(vh_surface);
  Dout(dc::vulkan, "Available present modes: " << available_present_modes);

  vk::SurfaceFormatKHR surface_format = choose_swap_surface_format(surface_formats);
  Dout(dc::vulkan, "Chosen surface format: " << surface_format);
  m_swapchain_image_format = surface_format.format;

  vk::PresentModeKHR present_mode = choose_swap_present_mode(available_present_modes);
  Dout(dc::vulkan, "Chosen present mode: " << present_mode);
  m_swapchain_extent = choose_swap_extent(surface_capabilities, m_window_extent);

  // Choose the number of swap chain images.
  uint32_t image_count = surface_capabilities.minImageCount + 1;
  if (surface_capabilities.maxImageCount > 0 && image_count > surface_capabilities.maxImageCount)
    image_count = surface_capabilities.maxImageCount;
  Dout(dc::vulkan, "Requesting " << image_count << " swap chain images (with extent " << m_swapchain_extent << ")");

  vk::SwapchainCreateInfoKHR create_info;
  create_info
    .setSurface(vh_surface)
    .setMinImageCount(image_count)
    .setImageFormat(surface_format.format)
    .setImageColorSpace(surface_format.colorSpace)
    .setImageExtent(m_swapchain_extent)
    .setImageArrayLayers(1)
    .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
    .setPreTransform(surface_capabilities.currentTransform)
    .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
    .setPresentMode(present_mode)
    .setClipped(true)
    ;

  if (graphics_queue.queue_family() == present_queue.queue_family())
    create_info
      .setImageSharingMode(vk::SharingMode::eExclusive)
      ;
  else
  {
    uint32_t queue_family_indices[] = { static_cast<uint32_t>(graphics_queue.queue_family().get_value()), static_cast<uint32_t>(present_queue.queue_family().get_value()) };
    create_info
      .setImageSharingMode(vk::SharingMode::eConcurrent)
      .setQueueFamilyIndexCount(2)
      .setPQueueFamilyIndices(queue_family_indices)
      ;
  }

  Dout(dc::vulkan, "Calling Device::createSwapchainKHR(" << create_info << ")");
  m_vh_swapchain = m_device->createSwapchainKHR(create_info);

  // This initialization should only take place here, and only once.
  ASSERT(m_vhv_swapchain_images.empty());
  m_vhv_swapchain_images = m_device->getSwapchainImagesKHR(m_vh_swapchain);

  m_swapchain_end = m_vhv_swapchain_images.iend();
  Dout(dc::vulkan, "Actual number of swap chain images: " << m_swapchain_end);
}

void HelloTriangleSwapchain::createImageViews()
{
  m_vhv_swapchain_image_views.resize(image_count());
  for (SwapchainIndex i(0); i != m_swapchain_end; ++i)
  {
    vk::ImageSubresourceRange image_subresource_range;
    image_subresource_range
      .setAspectMask(vk::ImageAspectFlagBits::eColor)
      .setBaseMipLevel(0)
      .setLevelCount(1)
      .setBaseArrayLayer(0)
      .setLayerCount(1);

    vk::ImageViewCreateInfo view_info;
    view_info
      .setImage(m_vhv_swapchain_images[i])
      .setViewType(vk::ImageViewType::e2D)
      .setFormat(m_swapchain_image_format)
      .setSubresourceRange(image_subresource_range);

    m_vhv_swapchain_image_views[i] = m_device->createImageView(view_info);
  }
}

void HelloTriangleSwapchain::createSyncObjects()
{
  m_vhv_image_available_semaphores.resize(image_count());
  m_vhv_render_finished_semaphores.resize(image_count());
  m_vhv_in_flight_fences.resize(image_count());
  m_vhv_images_in_flight.resize(image_count(), VK_NULL_HANDLE);

  vk::SemaphoreCreateInfo semaphore_create_info;

  vk::FenceCreateInfo fence_create_info;
  fence_create_info.flags = vk::FenceCreateFlagBits::eSignaled;

  for (SwapchainIndex i(0); i != m_swapchain_end; ++i)
  {
    m_vhv_image_available_semaphores[i] = m_device->createSemaphore(semaphore_create_info);
    m_vhv_render_finished_semaphores[i] = m_device->createSemaphore(semaphore_create_info);
    m_vhv_in_flight_fences[i] = m_device->createFence(fence_create_info);
  }
}

void HelloTriangleSwapchain::createRenderPass()
{
  vk::AttachmentDescription depth_attachment;
  depth_attachment
    .setFormat(find_depth_format())
    .setSamples(vk::SampleCountFlagBits::e1)
    .setLoadOp(vk::AttachmentLoadOp::eClear)
    .setStoreOp(vk::AttachmentStoreOp::eDontCare)
    .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
    .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
    .setInitialLayout(vk::ImageLayout::eUndefined)
    .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

  vk::AttachmentReference depth_attachment_ref;
  depth_attachment_ref
    .setAttachment(1)
    .setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

  vk::AttachmentDescription color_attachment;
  color_attachment
    .setFormat(get_swapchain_image_format())
    .setSamples(vk::SampleCountFlagBits::e1)
    .setLoadOp(vk::AttachmentLoadOp::eClear)
    .setStoreOp(vk::AttachmentStoreOp::eStore)
    .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
    .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
    .setInitialLayout(vk::ImageLayout::eUndefined)
    .setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

  vk::AttachmentReference color_attachment_ref;
  color_attachment_ref
    .setAttachment(0)
    .setLayout(vk::ImageLayout::eColorAttachmentOptimal);

  vk::SubpassDescription subpass;
  subpass
    .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
    .setColorAttachments(color_attachment_ref)
    .setPDepthStencilAttachment(&depth_attachment_ref);

  vk::SubpassDependency dependency;
  dependency
    .setSrcSubpass(VK_SUBPASS_EXTERNAL)
    .setSrcAccessMask(vk::AccessFlagBits::eNoneKHR)
    .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests)
    .setDstSubpass(0)
    .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests)
    .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite);

  std::array<vk::AttachmentDescription, 2> attachments = { color_attachment, depth_attachment };
  vk::RenderPassCreateInfo render_pass_info;
  render_pass_info
    .setAttachments(attachments)
    .setSubpasses(subpass)
    .setDependencies(dependency);

  m_vh_render_pass = m_device->createRenderPass(render_pass_info);
}

void HelloTriangleSwapchain::createDepthResources()
{
  vk::Format depth_format       = find_depth_format();
  vk::Extent2D swapchain_extent = get_swapchain_extent();

  m_vhv_depth_images.resize(image_count());
  m_vhv_depth_image_memorys.resize(image_count());
  m_vhv_depth_image_views.resize(image_count());

  for (SwapchainIndex i(0); i != m_swapchain_end; ++i)
  {
    vk::ImageCreateInfo image_create_info;
    image_create_info
      .setImageType(vk::ImageType::e2D)
      .setExtent({swapchain_extent.width, swapchain_extent.height, 1})
      .setMipLevels(1)
      .setArrayLayers(1)
      .setFormat(depth_format)
      .setTiling(vk::ImageTiling::eOptimal)
      .setInitialLayout(vk::ImageLayout::eUndefined)
      .setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment)
      .setSamples(vk::SampleCountFlagBits::e1)
      .setSharingMode(vk::SharingMode::eExclusive);

    // Fill m_vhv_depth_images[i] and m_vhv_depth_image_memorys[i].
    create_image_with_info(image_create_info, vk::MemoryPropertyFlagBits::eDeviceLocal, m_device,
        m_vhv_depth_images[i], m_vhv_depth_image_memorys[i]);

    vk::ImageSubresourceRange image_subresource_range;
    image_subresource_range
      .setAspectMask(vk::ImageAspectFlagBits::eDepth)
      .setBaseMipLevel(0)
      .setLevelCount(1)
      .setBaseArrayLayer(0)
      .setLayerCount(1);

    vk::ImageViewCreateInfo image_view_create_info;
    image_view_create_info
      .setImage(m_vhv_depth_images[i])
      .setViewType(vk::ImageViewType::e2D)
      .setFormat(depth_format)
      .setSubresourceRange(image_subresource_range);

    // Fill m_vhv_depth_image_views[i].
    m_vhv_depth_image_views[i] = m_device->createImageView(image_view_create_info);
  }
}

void HelloTriangleSwapchain::createFramebuffers()
{
  m_vhv_swapchain_framebuffers.resize(image_count());
  for (SwapchainIndex i(0); i != m_swapchain_end; ++i)
  {
    std::array<vk::ImageView, 2> attachments = { m_vhv_swapchain_image_views[i], m_vhv_depth_image_views[i] };

    vk::Extent2D swapchain_extent = get_swapchain_extent();
    vk::FramebufferCreateInfo framebuffer_create_info;
    framebuffer_create_info
      .setRenderPass(m_vh_render_pass)
      .setAttachments(attachments)
      .setWidth(swapchain_extent.width)
      .setHeight(swapchain_extent.height)
      .setLayers(1);

    m_vhv_swapchain_framebuffers[i] = m_device->createFramebuffer(framebuffer_create_info);
  }
}

//
// End of functions called by setup.
//=============================================================================

//=============================================================================
// Functions called by Window::drawFrame.
//

uint32_t HelloTriangleSwapchain::acquireNextImage()
{
  vk::Result wait_for_fences_result = m_device->waitForFences(1, &m_vhv_in_flight_fences[m_current_swapchain_index], true, std::numeric_limits<uint64_t>::max());
  if (wait_for_fences_result != vk::Result::eSuccess)
    throw std::runtime_error("Failed to wait for m_vhv_in_flight_fences!");

  auto rv = m_device->acquireNextImageKHR(m_vh_swapchain, std::numeric_limits<uint64_t>::max(),
      m_vhv_image_available_semaphores[m_current_swapchain_index],  // must be a not signaled semaphore
      {});

  if (rv.result != vk::Result::eSuccess && rv.result != vk::Result::eSuboptimalKHR)
    throw std::runtime_error("Failed to acquire swap chain image!");

  return rv.value;
}

void HelloTriangleSwapchain::submitCommandBuffers(vk::CommandBuffer const& buffers, SwapchainIndex const swapchain_index)
{
  if (m_vhv_images_in_flight[swapchain_index])
  {
    vk::Result wait_for_fences_result = m_device->waitForFences(1, &m_vhv_images_in_flight[swapchain_index], true, std::numeric_limits<uint64_t>::max());
    if (wait_for_fences_result != vk::Result::eSuccess)
      throw std::runtime_error("Failed to wait for m_vhv_in_flight_fences!");
  }
  m_vhv_images_in_flight[swapchain_index] = m_vhv_in_flight_fences[m_current_swapchain_index];

  vk::PipelineStageFlags const pipeline_stage_flags = vk::PipelineStageFlagBits::eColorAttachmentOutput;

  vk::SubmitInfo submit_info;
  submit_info
    .setWaitSemaphores(m_vhv_image_available_semaphores[m_current_swapchain_index])
    .setWaitDstStageMask(pipeline_stage_flags)
    .setCommandBuffers(buffers)
    .setSignalSemaphores(m_vhv_render_finished_semaphores[m_current_swapchain_index])
    ;

  vk::Result reset_fences_result = m_device->resetFences(1, &m_vhv_in_flight_fences[m_current_swapchain_index]);
  if (reset_fences_result != vk::Result::eSuccess)
    throw std::runtime_error("Failed to reset fence for m_vhv_in_flight_fences!");

  m_vh_graphics_queue.submit(submit_info, m_vhv_in_flight_fences[m_current_swapchain_index]);

  uint32_t const image_index = swapchain_index.get_value();
  vk::PresentInfoKHR present_info;
  present_info
    .setWaitSemaphores(m_vhv_render_finished_semaphores[m_current_swapchain_index])
    .setSwapchains(m_vh_swapchain)
    .setImageIndices(image_index)
    ;

  auto result = m_vh_present_queue.presentKHR(present_info);
  if (result != vk::Result::eSuccess)
    throw std::runtime_error("Failed to present swap chain image!");

  ++m_current_frame;
  m_current_swapchain_index = SwapchainIndex{m_current_frame % image_count()};
}

// End of functions called by Window::drawFrame.
//=============================================================================

//=============================================================================
// Private helper functions.

vk::Format HelloTriangleSwapchain::find_depth_format()
{
  return find_supported_format(
      { vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint }, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment,
      m_device.vh_physical_device());
}

} // namespace vulkan
