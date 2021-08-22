#include "sys.h"
#include "HelloTriangleSwapChain.h"
#include "SwapChainSupportDetails.h"
#include "debug.h"

// std
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

SwapChainSupportDetails getSwapChainSupport(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface)
{
  DoutEntering(dc::vulkan, "getSwapChainSupport(" << physical_device << ", " << surface << ")");

  SwapChainSupportDetails details;

  details.capabilities = physical_device.getSurfaceCapabilitiesKHR(surface);
  details.formats = physical_device.getSurfaceFormatsKHR(surface);
  details.present_modes = physical_device.getSurfacePresentModesKHR(surface);

  return details;
}

uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties, vk::PhysicalDevice physicalDevice)
{
  vk::PhysicalDeviceMemoryProperties memProperties;
  memProperties = physicalDevice.getMemoryProperties();
  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
  {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) { return i; }
  }

  throw std::runtime_error("failed to find suitable memory type!");
}

void createImageWithInfo(vk::ImageCreateInfo const& imageInfo, vk::MemoryPropertyFlags properties, vk::Image& image, vk::DeviceMemory& imageMemory, Device const& device)
{
  image = device->createImage(imageInfo);

  vk::MemoryRequirements memRequirements;
  device->getImageMemoryRequirements(image, &memRequirements);

  vk::MemoryAllocateInfo allocInfo{};
  allocInfo.allocationSize  = memRequirements.size;
  allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties, device.vh_physical_device());

  imageMemory = device->allocateMemory(allocInfo);

  device->bindImageMemory(image, imageMemory, 0);
}

vk::Extent2D chooseSwapExtent(vk::SurfaceCapabilitiesKHR const& capabilities, vk::Extent2D actualExtent)
{
  if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
  {
    return capabilities.currentExtent;
  }
  else
  {
    actualExtent.width      = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
    actualExtent.height     = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

    return actualExtent;
  }
}

vk::SurfaceFormatKHR chooseSwapSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const& availableFormats)
{
  for (auto const& availableFormat : availableFormats)
  {
    if (availableFormat.format == vk::Format::eB8G8R8A8Unorm && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
      return availableFormat;
  }

  // This should have thrown an exception before we got here.
  ASSERT(!availableFormats.empty());
  return availableFormats[0];
}

vk::PresentModeKHR chooseSwapPresentMode(std::vector<vk::PresentModeKHR> const& availablePresentModes)
{
  // for (auto const& availablePresentMode : availablePresentModes)
  // {
  //  if (availablePresentMode == vk::PresentModeKHR::eMailbox)
  //  {
  //    Dout(dc::vulkan, "Present mode: Mailbox");
  //    return availablePresentMode;
  //  }
  // }

  // for (auto const& availablePresentMode : availablePresentModes)
  // {
  //   if (availablePresentMode == vk::PresentModeKHR::eImmediate) {
  //     Dout(dc::vulkan, "Present mode: Immediate");
  //     return availablePresentMode;
  //   }
  // }

  Dout(dc::vulkan, "Present mode: V-Sync");
  return vk::PresentModeKHR::eFifo;
}

vk::Format find_supported_format(std::vector<vk::Format> const& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features, vk::PhysicalDevice physicalDevice)
{
  for (vk::Format format : candidates)
  {
    vk::FormatProperties props = physicalDevice.getFormatProperties(format);
    if ((tiling == vk::ImageTiling::eLinear  && (props.linearTilingFeatures & features) == features) ||
        (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features))
      return format;
  }
  throw std::runtime_error("failed to find supported format!");
}

} // namespace

// Constructor.
HelloTriangleSwapChain::HelloTriangleSwapChain(Device const& device) : m_device{device} { }

// And initialization.
void HelloTriangleSwapChain::setup(vk::Extent2D extent, Queue graphics_queue, Queue present_queue, vk::SurfaceKHR surface)
{
  DoutEntering(dc::vulkan, "HelloTriangleSwapChain::setup(" << extent << ", " << graphics_queue << ", " << present_queue << ", " << surface << ")");
  m_window_extent = extent;
  m_vh_graphics_queue = graphics_queue.vh_queue();
  m_vh_present_queue = present_queue.vh_queue();
  createSwapChain(surface, graphics_queue, present_queue);
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

void HelloTriangleSwapChain::createSwapChain(vk::SurfaceKHR surface, Queue graphics_queue, Queue present_queue)
{
  vk::PhysicalDevice physicalDevice = m_device.vh_physical_device();
  SwapChainSupportDetails swap_chain_support = getSwapChainSupport(physicalDevice, surface);

  vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swap_chain_support.formats);
  vk::PresentModeKHR presentMode     = chooseSwapPresentMode(swap_chain_support.present_modes);
  vk::Extent2D extent                = chooseSwapExtent(swap_chain_support.capabilities, m_window_extent);

  uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;
  if (swap_chain_support.capabilities.maxImageCount > 0 && image_count > swap_chain_support.capabilities.maxImageCount)
    image_count = swap_chain_support.capabilities.maxImageCount;

  vk::SwapchainCreateInfoKHR createInfo;
  createInfo
    .setSurface(surface)
    .setMinImageCount(image_count)
    .setImageFormat(surfaceFormat.format)
    .setImageColorSpace(surfaceFormat.colorSpace)
    .setImageExtent(extent)
    .setImageArrayLayers(1)
    .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
    .setPreTransform(swap_chain_support.capabilities.currentTransform)
    .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
    .setPresentMode(presentMode)
    .setClipped(true)
    ;

  if (graphics_queue.queue_family() == present_queue.queue_family())
    createInfo
      .setImageSharingMode(vk::SharingMode::eExclusive)
      ;
  else
  {
    uint32_t queueFamilyIndices[] = { static_cast<uint32_t>(graphics_queue.queue_family().get_value()), static_cast<uint32_t>(present_queue.queue_family().get_value()) };
    createInfo
      .setImageSharingMode(vk::SharingMode::eConcurrent)
      .setQueueFamilyIndexCount(2)
      .setPQueueFamilyIndices(queueFamilyIndices)
      ;
  }

  m_vh_swap_chain = m_device->createSwapchainKHR(createInfo);
  m_vhv_swap_chain_images = m_device->getSwapchainImagesKHR(m_vh_swap_chain);
  m_swap_chain_image_format = surfaceFormat.format;
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
  vk::AttachmentDescription depthAttachment{};
  depthAttachment.format         = find_depth_format();
  depthAttachment.samples        = vk::SampleCountFlagBits::e1;
  depthAttachment.loadOp         = vk::AttachmentLoadOp::eClear;
  depthAttachment.storeOp        = vk::AttachmentStoreOp::eDontCare;
  depthAttachment.stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;
  depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
  depthAttachment.initialLayout  = vk::ImageLayout::eUndefined;
  depthAttachment.finalLayout    = vk::ImageLayout::eDepthStencilAttachmentOptimal;

  vk::AttachmentReference depthAttachmentRef{};
  depthAttachmentRef.attachment = 1;
  depthAttachmentRef.layout     = vk::ImageLayout::eDepthStencilAttachmentOptimal;

  vk::AttachmentDescription colorAttachment;
  colorAttachment.format                  = get_swap_chain_image_format();
  colorAttachment.samples                 = vk::SampleCountFlagBits::e1;
  colorAttachment.loadOp                  = vk::AttachmentLoadOp::eClear;
  colorAttachment.storeOp                 = vk::AttachmentStoreOp::eStore;
  colorAttachment.stencilStoreOp          = vk::AttachmentStoreOp::eDontCare;
  colorAttachment.stencilLoadOp           = vk::AttachmentLoadOp::eDontCare;
  colorAttachment.initialLayout           = vk::ImageLayout::eUndefined;
  colorAttachment.finalLayout             = vk::ImageLayout::ePresentSrcKHR;

  vk::AttachmentReference colorAttachmentRef;
  colorAttachmentRef.attachment            = 0;
  colorAttachmentRef.layout                = vk::ImageLayout::eColorAttachmentOptimal;

  vk::SubpassDescription subpass;
  subpass.pipelineBindPoint       = vk::PipelineBindPoint::eGraphics;
  subpass.colorAttachmentCount    = 1;
  subpass.pColorAttachments       = &colorAttachmentRef;
  subpass.pDepthStencilAttachment = &depthAttachmentRef;

  vk::SubpassDependency dependency;
  dependency.srcSubpass          = VK_SUBPASS_EXTERNAL;
  dependency.srcAccessMask       = vk::AccessFlagBits::eNoneKHR;
  dependency.srcStageMask        = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
  dependency.dstSubpass          = 0;
  dependency.dstStageMask        = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
  dependency.dstAccessMask       = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite;

  std::array<vk::AttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
  vk::RenderPassCreateInfo renderPassInfo;
  renderPassInfo.attachmentCount                     = static_cast<uint32_t>(attachments.size());
  renderPassInfo.pAttachments                        = attachments.data();
  renderPassInfo.subpassCount                        = 1;
  renderPassInfo.pSubpasses                          = &subpass;
  renderPassInfo.dependencyCount                     = 1;
  renderPassInfo.pDependencies                       = &dependency;

  m_vh_render_pass = m_device->createRenderPass(renderPassInfo);
}

void HelloTriangleSwapChain::createDepthResources()
{
  vk::Format depthFormat       = find_depth_format();
  vk::Extent2D swapChainExtent = get_swap_chain_extent();

  m_vhv_depth_images.resize(image_count());
  m_vhv_depth_image_memorys.resize(image_count());
  m_vhv_depth_image_views.resize(image_count());

  for (int i = 0; i < m_vhv_depth_images.size(); ++i)
  {
    vk::ImageCreateInfo imageInfo;
    imageInfo.imageType     = vk::ImageType::e2D;
    imageInfo.extent.width  = swapChainExtent.width;
    imageInfo.extent.height = swapChainExtent.height;
    imageInfo.extent.depth  = 1;
    imageInfo.mipLevels     = 1;
    imageInfo.arrayLayers   = 1;
    imageInfo.format        = depthFormat;
    imageInfo.tiling        = vk::ImageTiling::eOptimal;
    imageInfo.initialLayout = vk::ImageLayout::eUndefined;
    imageInfo.usage         = vk::ImageUsageFlagBits::eDepthStencilAttachment;
    imageInfo.samples       = vk::SampleCountFlagBits::e1;
    imageInfo.sharingMode   = vk::SharingMode::eExclusive;
//    imageInfo.flags         = 0;

    createImageWithInfo(imageInfo, vk::MemoryPropertyFlagBits::eDeviceLocal, m_vhv_depth_images[i], m_vhv_depth_image_memorys[i], m_device);

    vk::ImageViewCreateInfo viewInfo;
    viewInfo.image                           = m_vhv_depth_images[i];
    viewInfo.viewType                        = vk::ImageViewType::e2D;
    viewInfo.format                          = depthFormat;
    viewInfo.subresourceRange.aspectMask     = vk::ImageAspectFlagBits::eDepth;
    viewInfo.subresourceRange.baseMipLevel   = 0;
    viewInfo.subresourceRange.levelCount     = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount     = 1;

    m_vhv_depth_image_views[i] = m_device->createImageView(viewInfo);
  }
}

void HelloTriangleSwapChain::createFramebuffers()
{
  m_vhv_swap_chain_framebuffers.resize(image_count());
  for (size_t i = 0; i < image_count(); ++i)
  {
    std::array<vk::ImageView, 2> attachments = {m_vhv_swap_chain_image_views[i], m_vhv_depth_image_views[i]};

    vk::Extent2D swap_chain_extent          = get_swap_chain_extent();
    vk::FramebufferCreateInfo framebufferInfo;
    framebufferInfo.renderPass              = m_vh_render_pass;
    framebufferInfo.attachmentCount         = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments            = attachments.data();
    framebufferInfo.width                   = swap_chain_extent.width;
    framebufferInfo.height                  = swap_chain_extent.height;
    framebufferInfo.layers                  = 1;

    m_vhv_swap_chain_framebuffers[i] = m_device->createFramebuffer(framebufferInfo);
  }
}

void HelloTriangleSwapChain::createSyncObjects()
{
  m_vhv_image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
  m_vhv_render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
  m_vhv_in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);
  m_vhv_images_in_flight.resize(image_count(), VK_NULL_HANDLE);

  vk::SemaphoreCreateInfo semaphoreInfo;

  vk::FenceCreateInfo fenceInfo;
  fenceInfo.flags             = vk::FenceCreateFlagBits::eSignaled;

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
  {
    m_vhv_image_available_semaphores[i] = m_device->createSemaphore(semaphoreInfo);
    m_vhv_render_finished_semaphores[i] = m_device->createSemaphore(semaphoreInfo);
    m_vhv_in_flight_fences[i] = m_device->createFence(fenceInfo);
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

  vk::SubmitInfo submitInfo;
  submitInfo
    .setWaitSemaphores(m_vhv_image_available_semaphores[m_current_frame])
    .setWaitDstStageMask(pipeline_stage_flags)
    .setCommandBuffers(buffers)
    .setSignalSemaphores(m_vhv_render_finished_semaphores[m_current_frame])
    ;

  vk::Result reset_fences_result = m_device->resetFences(1, &m_vhv_in_flight_fences[m_current_frame]);
  if (reset_fences_result != vk::Result::eSuccess)
    throw std::runtime_error("Failed to reset fence for m_vhv_in_flight_fences!");

  m_vh_graphics_queue.submit(submitInfo, m_vhv_in_flight_fences[m_current_frame]);

  vk::PresentInfoKHR presentInfo;
  presentInfo
    .setWaitSemaphores(m_vhv_render_finished_semaphores[m_current_frame])
    .setSwapchains(m_vh_swap_chain)
    .setImageIndices(imageIndex)
    ;

  auto result = m_vh_present_queue.presentKHR(presentInfo);
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
