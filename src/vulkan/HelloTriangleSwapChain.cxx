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

namespace vulkan {

HelloTriangleSwapChain::HelloTriangleSwapChain(Device const& deviceRef) : device{deviceRef} { }

void HelloTriangleSwapChain::setup(vk::Extent2D extent, Queue graphics_queue, Queue present_queue, vk::SurfaceKHR surface)
{
  windowExtent = extent;
  m_graphics_queue = graphics_queue.vk_handle();
  m_present_queue = present_queue.vk_handle();
  createSwapChain(surface, graphics_queue, present_queue);
  createImageViews();
  createRenderPass();
  createDepthResources();
  createFramebuffers();
  createSyncObjects();
}

HelloTriangleSwapChain::~HelloTriangleSwapChain()
{
  for (auto imageView : swapChainImageViews) { device->destroyImageView(imageView); }
  swapChainImageViews.clear();

  if (swapChain)
  {
    device->destroySwapchainKHR(swapChain);
    swapChain = nullptr;
  }

  for (int i = 0; i < depthImages.size(); ++i)
  {
    device->destroyImageView(depthImageViews[i]);
    device->destroyImage(depthImages[i]);
    device->freeMemory(depthImageMemorys[i]);
  }

  for (auto framebuffer : swapChainFramebuffers) { device->destroyFramebuffer(framebuffer); }

  device->destroyRenderPass(renderPass);

  // cleanup synchronization objects
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
  {
    device->destroySemaphore(renderFinishedSemaphores[i]);
    device->destroySemaphore(imageAvailableSemaphores[i]);
    device->destroyFence(inFlightFences[i]);
  }
}

uint32_t HelloTriangleSwapChain::acquireNextImage()
{
  vk::Result wait_for_fences_result = device->waitForFences(1, &inFlightFences[currentFrame], true, std::numeric_limits<uint64_t>::max());
  if (wait_for_fences_result != vk::Result::eSuccess)
    throw std::runtime_error("Failed to wait for inFlightFences!");

  auto rv = device->acquireNextImageKHR(swapChain, std::numeric_limits<uint64_t>::max(),
      imageAvailableSemaphores[currentFrame],  // must be a not signaled semaphore
      {});

  if (rv.result != vk::Result::eSuccess && rv.result != vk::Result::eSuboptimalKHR)
    throw std::runtime_error("Failed to acquire swap chain image!");

  return rv.value;
}

void HelloTriangleSwapChain::submitCommandBuffers(vk::CommandBuffer const& buffers, uint32_t const imageIndex)
{
  if (imagesInFlight[imageIndex])
  {
    vk::Result wait_for_fences_result = device->waitForFences(1, &imagesInFlight[imageIndex], true, std::numeric_limits<uint64_t>::max());
    if (wait_for_fences_result != vk::Result::eSuccess)
      throw std::runtime_error("Failed to wait for inFlightFences!");
  }
  imagesInFlight[imageIndex] = inFlightFences[currentFrame];

  vk::PipelineStageFlags const pipeline_stage_flags = vk::PipelineStageFlagBits::eColorAttachmentOutput;

  vk::SubmitInfo submitInfo;
  submitInfo
    .setWaitSemaphores(imageAvailableSemaphores[currentFrame])
    .setWaitDstStageMask(pipeline_stage_flags)
    .setCommandBuffers(buffers)
    .setSignalSemaphores(renderFinishedSemaphores[currentFrame])
    ;

  vk::Result reset_fences_result = device->resetFences(1, &inFlightFences[currentFrame]);
  if (reset_fences_result != vk::Result::eSuccess)
    throw std::runtime_error("Failed to reset fence for inFlightFences!");

  m_graphics_queue.submit(submitInfo, inFlightFences[currentFrame]);

  vk::PresentInfoKHR presentInfo;
  presentInfo
    .setWaitSemaphores(renderFinishedSemaphores[currentFrame])
    .setSwapchains(swapChain)
    .setImageIndices(imageIndex)
    ;

  auto result = m_present_queue.presentKHR(presentInfo);
  if (result != vk::Result::eSuccess)
    throw std::runtime_error("Failed to present swap chain image!");

  currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

SwapChainSupportDetails getSwapChainSupport(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface)
{
  SwapChainSupportDetails details;

  details.capabilities = physical_device.getSurfaceCapabilitiesKHR(surface);
  details.formats = physical_device.getSurfaceFormatsKHR(surface);
  details.presentModes = physical_device.getSurfacePresentModesKHR(surface);

  return details;
}

void HelloTriangleSwapChain::createSwapChain(vk::SurfaceKHR surface, Queue graphics_queue, Queue present_queue)
{
  vk::PhysicalDevice physicalDevice = device.get_physical_device();
  SwapChainSupportDetails swapChainSupport = getSwapChainSupport(physicalDevice, surface);

  vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
  vk::PresentModeKHR presentMode     = chooseSwapPresentMode(swapChainSupport.presentModes);
  vk::Extent2D extent                = chooseSwapExtent(swapChainSupport.capabilities);

  uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
  if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
    imageCount = swapChainSupport.capabilities.maxImageCount;

  vk::SwapchainCreateInfoKHR createInfo;
  createInfo
    .setSurface(surface)
    .setMinImageCount(imageCount)
    .setImageFormat(surfaceFormat.format)
    .setImageColorSpace(surfaceFormat.colorSpace)
    .setImageExtent(extent)
    .setImageArrayLayers(1)
    .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
    .setPreTransform(swapChainSupport.capabilities.currentTransform)
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

  swapChain = device->createSwapchainKHR(createInfo);
  swapChainImages = device->getSwapchainImagesKHR(swapChain);
  swapChainImageFormat = surfaceFormat.format;
  swapChainExtent      = extent;
}

void HelloTriangleSwapChain::createImageViews()
{
  swapChainImageViews.resize(swapChainImages.size());
  for (size_t i = 0; i < swapChainImages.size(); ++i)
  {
    vk::ImageViewCreateInfo viewInfo;
    viewInfo.image                           = swapChainImages[i];
    viewInfo.viewType                        = vk::ImageViewType::e2D;
    viewInfo.format                          = swapChainImageFormat;
    viewInfo.subresourceRange.aspectMask     = vk::ImageAspectFlagBits::eColor;
    viewInfo.subresourceRange.baseMipLevel   = 0;
    viewInfo.subresourceRange.levelCount     = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount     = 1;

    swapChainImageViews[i] = device->createImageView(viewInfo);
  }
}

void HelloTriangleSwapChain::createRenderPass()
{
  vk::AttachmentDescription depthAttachment{};
  depthAttachment.format         = findDepthFormat();
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
  colorAttachment.format                  = getSwapChainImageFormat();
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

  renderPass = device->createRenderPass(renderPassInfo);
}

void HelloTriangleSwapChain::createFramebuffers()
{
  swapChainFramebuffers.resize(imageCount());
  for (size_t i = 0; i < imageCount(); ++i)
  {
    std::array<vk::ImageView, 2> attachments = {swapChainImageViews[i], depthImageViews[i]};

    vk::Extent2D swapChainExtent              = getSwapChainExtent();
    vk::FramebufferCreateInfo framebufferInfo;
    framebufferInfo.renderPass              = renderPass;
    framebufferInfo.attachmentCount         = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments            = attachments.data();
    framebufferInfo.width                   = swapChainExtent.width;
    framebufferInfo.height                  = swapChainExtent.height;
    framebufferInfo.layers                  = 1;

    swapChainFramebuffers[i] = device->createFramebuffer(framebufferInfo);
  }
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
  allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties, device.get_physical_device());

  imageMemory = device->allocateMemory(allocInfo);

  device->bindImageMemory(image, imageMemory, 0);
}

void HelloTriangleSwapChain::createDepthResources()
{
  vk::Format depthFormat       = findDepthFormat();
  vk::Extent2D swapChainExtent = getSwapChainExtent();

  depthImages.resize(imageCount());
  depthImageMemorys.resize(imageCount());
  depthImageViews.resize(imageCount());

  for (int i = 0; i < depthImages.size(); ++i)
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

    createImageWithInfo(imageInfo, vk::MemoryPropertyFlagBits::eDeviceLocal, depthImages[i], depthImageMemorys[i], device);

    vk::ImageViewCreateInfo viewInfo;
    viewInfo.image                           = depthImages[i];
    viewInfo.viewType                        = vk::ImageViewType::e2D;
    viewInfo.format                          = depthFormat;
    viewInfo.subresourceRange.aspectMask     = vk::ImageAspectFlagBits::eDepth;
    viewInfo.subresourceRange.baseMipLevel   = 0;
    viewInfo.subresourceRange.levelCount     = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount     = 1;

    depthImageViews[i] = device->createImageView(viewInfo);
  }
}

void HelloTriangleSwapChain::createSyncObjects()
{
  imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
  imagesInFlight.resize(imageCount(), VK_NULL_HANDLE);

  vk::SemaphoreCreateInfo semaphoreInfo;

  vk::FenceCreateInfo fenceInfo;
  fenceInfo.flags             = vk::FenceCreateFlagBits::eSignaled;

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
  {
    imageAvailableSemaphores[i] = device->createSemaphore(semaphoreInfo);
    renderFinishedSemaphores[i] = device->createSemaphore(semaphoreInfo);
    inFlightFences[i] = device->createFence(fenceInfo);
  }
}

vk::SurfaceFormatKHR HelloTriangleSwapChain::chooseSwapSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const& availableFormats)
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

vk::PresentModeKHR HelloTriangleSwapChain::chooseSwapPresentMode(std::vector<vk::PresentModeKHR> const& availablePresentModes)
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

vk::Extent2D HelloTriangleSwapChain::chooseSwapExtent(vk::SurfaceCapabilitiesKHR const& capabilities)
{
  if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) { return capabilities.currentExtent; }
  else
  {
    vk::Extent2D actualExtent = windowExtent;
    actualExtent.width      = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
    actualExtent.height     = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

    return actualExtent;
  }
}

vk::Format findSupportedFormat(std::vector<vk::Format> const& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features, vk::PhysicalDevice physicalDevice)
{
  for (vk::Format format : candidates)
  {
    vk::FormatProperties props;
    props = physicalDevice.getFormatProperties(format);

    if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) { return format; }
    else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features)
    {
      return format;
    }
  }
  throw std::runtime_error("failed to find supported format!");
}

vk::Format HelloTriangleSwapChain::findDepthFormat()
{
  return findSupportedFormat(
      { vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint }, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment,
      device.get_physical_device());
}

} // namespace vulkan
