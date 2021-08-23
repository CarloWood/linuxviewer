#include "sys.h"
#include "HelloTriangleSwapChain.h"
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

uint32_t find_memory_type(uint32_t typeFilter, vk::MemoryPropertyFlags properties, vk::PhysicalDevice vh_physical_device)
{
  vk::PhysicalDeviceMemoryProperties memProperties;
  memProperties = vh_physical_device.getMemoryProperties();
  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
  {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) { return i; }
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

vk::SurfaceFormatKHR choose_swap_surface_format(std::vector<vk::SurfaceFormatKHR> const& available_formats)
{
  for (auto const& available_format : available_formats)
  {
    if (available_format.format == vk::Format::eB8G8R8A8Unorm && available_format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
      return available_format;
  }

  // This should have thrown an exception before we got here.
  ASSERT(!available_formats.empty());

  return available_formats[0];
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
HelloTriangleSwapChain::HelloTriangleSwapChain(Device const& device) : m_device{device} { }

// And initialization.
void HelloTriangleSwapChain::setup(vk::Extent2D extent, Queue graphics_queue, Queue present_queue, vk::SurfaceKHR vh_surface)
{
  DoutEntering(dc::vulkan, "HelloTriangleSwapChain::setup(" << extent << ", " << graphics_queue << ", " << present_queue << ", " << vh_surface << ")");
  m_window_extent = extent;
  m_vh_graphics_queue = graphics_queue.vh_queue();
  m_vh_present_queue = present_queue.vh_queue();
  createSwapChain(vh_surface, graphics_queue, present_queue);
  createImageViews();
  createRenderPass();
  createDepthResources();
  createFramebuffers();
  createSyncObjects();
}

// Destructor.
HelloTriangleSwapChain::~HelloTriangleSwapChain()
{
  for (auto image_view : m_vhv_swap_chain_image_views) { m_device->destroyImageView(image_view); }
  m_vhv_swap_chain_image_views.clear();

  if (m_vh_swap_chain)
  {
    m_device->destroySwapchainKHR(m_vh_swap_chain);
    m_vh_swap_chain = nullptr;
  }

  for (int i = 0; i < m_vhv_depth_images.size(); ++i)
  {
    m_device->destroyImageView(m_vhv_depth_image_views[i]);
    m_device->destroyImage(m_vhv_depth_images[i]);
    m_device->freeMemory(m_vhv_depth_image_memorys[i]);
  }

  for (auto framebuffer : m_vhv_swap_chain_framebuffers) { m_device->destroyFramebuffer(framebuffer); }

  m_device->destroyRenderPass(m_vh_render_pass);

  // cleanup synchronization objects
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
  {
    m_device->destroySemaphore(m_vhv_render_finished_semaphores[i]);
    m_device->destroySemaphore(m_vhv_image_available_semaphores[i]);
    m_device->destroyFence(m_vhv_in_flight_fences[i]);
  }
}

//=============================================================================
// Functions called by setup
//

void HelloTriangleSwapChain::createSwapChain(vk::SurfaceKHR vh_surface, Queue graphics_queue, Queue present_queue)
{
  DoutEntering(dc::vulkan, "HelloTriangleSwapChain::createSwapChain(" << vh_surface << ", " << graphics_queue << ", " << present_queue << ")");

  // Query supported surface details.
  vk::PhysicalDevice vh_physical_device = m_device.vh_physical_device();
  vk::SurfaceCapabilitiesKHR surface_capabilities = vh_physical_device.getSurfaceCapabilitiesKHR(vh_surface);
  Dout(dc::vulkan, "surface_capabilities: " << surface_capabilities);
  std::vector<vk::SurfaceFormatKHR> surface_formats = vh_physical_device.getSurfaceFormatsKHR(vh_surface);
  Dout(dc::vulkan, "surface_formats: " << surface_formats);
  std::vector<vk::PresentModeKHR> available_present_modes = vh_physical_device.getSurfacePresentModesKHR(vh_surface);
  Dout(dc::vulkan, "available_present_modes: " << available_present_modes);

  vk::SurfaceFormatKHR surface_format = choose_swap_surface_format(surface_formats);
  vk::PresentModeKHR present_mode     = choose_swap_present_mode(available_present_modes);
  vk::Extent2D extent                 = choose_swap_extent(surface_capabilities, m_window_extent);

  uint32_t image_count = surface_capabilities.minImageCount + 1;
  if (surface_capabilities.maxImageCount > 0 && image_count > surface_capabilities.maxImageCount)
    image_count = surface_capabilities.maxImageCount;

  vk::SwapchainCreateInfoKHR create_info;
  create_info
    .setSurface(vh_surface)
    .setMinImageCount(image_count)
    .setImageFormat(surface_format.format)
    .setImageColorSpace(surface_format.colorSpace)
    .setImageExtent(extent)
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
    uint32_t queueFamilyIndices[] = { static_cast<uint32_t>(graphics_queue.queue_family().get_value()), static_cast<uint32_t>(present_queue.queue_family().get_value()) };
    create_info
      .setImageSharingMode(vk::SharingMode::eConcurrent)
      .setQueueFamilyIndexCount(2)
      .setPQueueFamilyIndices(queueFamilyIndices)
      ;
  }

  m_vh_swap_chain = m_device->createSwapchainKHR(create_info);
  m_vhv_swap_chain_images = m_device->getSwapchainImagesKHR(m_vh_swap_chain);
  m_swap_chain_image_format = surface_format.format;
  m_swap_chain_extent      = extent;
}

void HelloTriangleSwapChain::createImageViews()
{
  m_vhv_swap_chain_image_views.resize(m_vhv_swap_chain_images.size());
  for (size_t i = 0; i < m_vhv_swap_chain_images.size(); ++i)
  {
    vk::ImageViewCreateInfo viewInfo;
    viewInfo.image                           = m_vhv_swap_chain_images[i];
    viewInfo.viewType                        = vk::ImageViewType::e2D;
    viewInfo.format                          = m_swap_chain_image_format;
    viewInfo.subresourceRange.aspectMask     = vk::ImageAspectFlagBits::eColor;
    viewInfo.subresourceRange.baseMipLevel   = 0;
    viewInfo.subresourceRange.levelCount     = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount     = 1;

    m_vhv_swap_chain_image_views[i] = m_device->createImageView(viewInfo);
  }
}

void HelloTriangleSwapChain::createRenderPass()
{
  vk::AttachmentDescription depth_attachment{};
  depth_attachment.format         = find_depth_format();
  depth_attachment.samples        = vk::SampleCountFlagBits::e1;
  depth_attachment.loadOp         = vk::AttachmentLoadOp::eClear;
  depth_attachment.storeOp        = vk::AttachmentStoreOp::eDontCare;
  depth_attachment.stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;
  depth_attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
  depth_attachment.initialLayout  = vk::ImageLayout::eUndefined;
  depth_attachment.finalLayout    = vk::ImageLayout::eDepthStencilAttachmentOptimal;

  vk::AttachmentReference depth_attachment_ref{};
  depth_attachment_ref.attachment = 1;
  depth_attachment_ref.layout     = vk::ImageLayout::eDepthStencilAttachmentOptimal;

  vk::AttachmentDescription color_attachment;
  color_attachment.format                  = get_swap_chain_image_format();
  color_attachment.samples                 = vk::SampleCountFlagBits::e1;
  color_attachment.loadOp                  = vk::AttachmentLoadOp::eClear;
  color_attachment.storeOp                 = vk::AttachmentStoreOp::eStore;
  color_attachment.stencilStoreOp          = vk::AttachmentStoreOp::eDontCare;
  color_attachment.stencilLoadOp           = vk::AttachmentLoadOp::eDontCare;
  color_attachment.initialLayout           = vk::ImageLayout::eUndefined;
  color_attachment.finalLayout             = vk::ImageLayout::ePresentSrcKHR;

  vk::AttachmentReference color_attachment_ref;
  color_attachment_ref.attachment            = 0;
  color_attachment_ref.layout                = vk::ImageLayout::eColorAttachmentOptimal;

  vk::SubpassDescription subpass;
  subpass.pipelineBindPoint       = vk::PipelineBindPoint::eGraphics;
  subpass.colorAttachmentCount    = 1;
  subpass.pColorAttachments       = &color_attachment_ref;
  subpass.pDepthStencilAttachment = &depth_attachment_ref;

  vk::SubpassDependency dependency;
  dependency.srcSubpass          = VK_SUBPASS_EXTERNAL;
  dependency.srcAccessMask       = vk::AccessFlagBits::eNoneKHR;
  dependency.srcStageMask        = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
  dependency.dstSubpass          = 0;
  dependency.dstStageMask        = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
  dependency.dstAccessMask       = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite;

  std::array<vk::AttachmentDescription, 2> attachments = { color_attachment, depth_attachment };
  vk::RenderPassCreateInfo render_pass_info;
  render_pass_info.attachmentCount                     = static_cast<uint32_t>(attachments.size());
  render_pass_info.pAttachments                        = attachments.data();
  render_pass_info.subpassCount                        = 1;
  render_pass_info.pSubpasses                          = &subpass;
  render_pass_info.dependencyCount                     = 1;
  render_pass_info.pDependencies                       = &dependency;

  m_vh_render_pass = m_device->createRenderPass(render_pass_info);
}

void HelloTriangleSwapChain::createDepthResources()
{
  vk::Format depth_format       = find_depth_format();
  vk::Extent2D swap_chain_extent = get_swap_chain_extent();

  m_vhv_depth_images.resize(image_count());
  m_vhv_depth_image_memorys.resize(image_count());
  m_vhv_depth_image_views.resize(image_count());

  for (int i = 0; i < m_vhv_depth_images.size(); ++i)
  {
    vk::ImageCreateInfo image_create_info;
    image_create_info.imageType     = vk::ImageType::e2D;
    image_create_info.extent.width  = swap_chain_extent.width;
    image_create_info.extent.height = swap_chain_extent.height;
    image_create_info.extent.depth  = 1;
    image_create_info.mipLevels     = 1;
    image_create_info.arrayLayers   = 1;
    image_create_info.format        = depth_format;
    image_create_info.tiling        = vk::ImageTiling::eOptimal;
    image_create_info.initialLayout = vk::ImageLayout::eUndefined;
    image_create_info.usage         = vk::ImageUsageFlagBits::eDepthStencilAttachment;
    image_create_info.samples       = vk::SampleCountFlagBits::e1;
    image_create_info.sharingMode   = vk::SharingMode::eExclusive;

    // Fill m_vhv_depth_images[i] and m_vhv_depth_image_memorys[i].
    create_image_with_info(image_create_info, vk::MemoryPropertyFlagBits::eDeviceLocal, m_device,
        m_vhv_depth_images[i], m_vhv_depth_image_memorys[i]);

    vk::ImageViewCreateInfo image_view_create_info;
    image_view_create_info.image                           = m_vhv_depth_images[i];
    image_view_create_info.viewType                        = vk::ImageViewType::e2D;
    image_view_create_info.format                          = depth_format;
    image_view_create_info.subresourceRange.aspectMask     = vk::ImageAspectFlagBits::eDepth;
    image_view_create_info.subresourceRange.baseMipLevel   = 0;
    image_view_create_info.subresourceRange.levelCount     = 1;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount     = 1;

    m_vhv_depth_image_views[i] = m_device->createImageView(image_view_create_info);
  }
}

void HelloTriangleSwapChain::createFramebuffers()
{
  m_vhv_swap_chain_framebuffers.resize(image_count());
  for (size_t i = 0; i < image_count(); ++i)
  {
    std::array<vk::ImageView, 2> attachments = { m_vhv_swap_chain_image_views[i], m_vhv_depth_image_views[i] };

    vk::Extent2D swap_chain_extent = get_swap_chain_extent();
    vk::FramebufferCreateInfo framebuffer_create_info;
    framebuffer_create_info.renderPass      = m_vh_render_pass;
    framebuffer_create_info.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebuffer_create_info.pAttachments    = attachments.data();
    framebuffer_create_info.width           = swap_chain_extent.width;
    framebuffer_create_info.height          = swap_chain_extent.height;
    framebuffer_create_info.layers          = 1;

    m_vhv_swap_chain_framebuffers[i] = m_device->createFramebuffer(framebuffer_create_info);
  }
}

void HelloTriangleSwapChain::createSyncObjects()
{
  m_vhv_image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
  m_vhv_render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
  m_vhv_in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);
  m_vhv_images_in_flight.resize(image_count(), VK_NULL_HANDLE);

  vk::SemaphoreCreateInfo semaphore_create_info;

  vk::FenceCreateInfo fence_create_info;
  fence_create_info.flags = vk::FenceCreateFlagBits::eSignaled;

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
  {
    m_vhv_image_available_semaphores[i] = m_device->createSemaphore(semaphore_create_info);
    m_vhv_render_finished_semaphores[i] = m_device->createSemaphore(semaphore_create_info);
    m_vhv_in_flight_fences[i] = m_device->createFence(fence_create_info);
  }
}

//
// End of functions called by setup.
//=============================================================================

//=============================================================================
// Functions called by Window::drawFrame.
//

uint32_t HelloTriangleSwapChain::acquireNextImage()
{
  vk::Result wait_for_fences_result = m_device->waitForFences(1, &m_vhv_in_flight_fences[m_current_frame], true, std::numeric_limits<uint64_t>::max());
  if (wait_for_fences_result != vk::Result::eSuccess)
    throw std::runtime_error("Failed to wait for m_vhv_in_flight_fences!");

  auto rv = m_device->acquireNextImageKHR(m_vh_swap_chain, std::numeric_limits<uint64_t>::max(),
      m_vhv_image_available_semaphores[m_current_frame],  // must be a not signaled semaphore
      {});

  if (rv.result != vk::Result::eSuccess && rv.result != vk::Result::eSuboptimalKHR)
    throw std::runtime_error("Failed to acquire swap chain image!");

  return rv.value;
}

void HelloTriangleSwapChain::submitCommandBuffers(vk::CommandBuffer const& buffers, uint32_t const imageIndex)
{
  if (m_vhv_images_in_flight[imageIndex])
  {
    vk::Result wait_for_fences_result = m_device->waitForFences(1, &m_vhv_images_in_flight[imageIndex], true, std::numeric_limits<uint64_t>::max());
    if (wait_for_fences_result != vk::Result::eSuccess)
      throw std::runtime_error("Failed to wait for m_vhv_in_flight_fences!");
  }
  m_vhv_images_in_flight[imageIndex] = m_vhv_in_flight_fences[m_current_frame];

  vk::PipelineStageFlags const pipeline_stage_flags = vk::PipelineStageFlagBits::eColorAttachmentOutput;

  vk::SubmitInfo submit_info;
  submit_info
    .setWaitSemaphores(m_vhv_image_available_semaphores[m_current_frame])
    .setWaitDstStageMask(pipeline_stage_flags)
    .setCommandBuffers(buffers)
    .setSignalSemaphores(m_vhv_render_finished_semaphores[m_current_frame])
    ;

  vk::Result reset_fences_result = m_device->resetFences(1, &m_vhv_in_flight_fences[m_current_frame]);
  if (reset_fences_result != vk::Result::eSuccess)
    throw std::runtime_error("Failed to reset fence for m_vhv_in_flight_fences!");

  m_vh_graphics_queue.submit(submit_info, m_vhv_in_flight_fences[m_current_frame]);

  vk::PresentInfoKHR present_info;
  present_info
    .setWaitSemaphores(m_vhv_render_finished_semaphores[m_current_frame])
    .setSwapchains(m_vh_swap_chain)
    .setImageIndices(imageIndex)
    ;

  auto result = m_vh_present_queue.presentKHR(present_info);
  if (result != vk::Result::eSuccess)
    throw std::runtime_error("Failed to present swap chain image!");

  m_current_frame = (m_current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

// End of functions called by Window::drawFrame.
//=============================================================================

//=============================================================================
// Private helper functions.

vk::Format HelloTriangleSwapChain::find_depth_format()
{
  return find_supported_format(
      { vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint }, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment,
      m_device.vh_physical_device());
}

} // namespace vulkan
