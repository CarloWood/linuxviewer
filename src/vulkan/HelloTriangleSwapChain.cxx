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
  m_graphics_queue = graphics_queue.index();
  m_present_queue = present_queue.index();
  createSwapChain(surface, graphics_queue, present_queue);
  createImageViews();
  createRenderPass();
  createDepthResources();
  createFramebuffers();
  createSyncObjects();
}

HelloTriangleSwapChain::~HelloTriangleSwapChain()
{
  for (auto imageView : swapChainImageViews) { device.device().destroyImageView(imageView); }
  swapChainImageViews.clear();

  if (swapChain)
  {
    device.device().destroySwapchainKHR(swapChain);
    swapChain = nullptr;
  }

  for (int i = 0; i < depthImages.size(); ++i)
  {
    device.device().destroyImageView(depthImageViews[i]);
    device.device().destroyImage(depthImages[i]);
    device.device().freeMemory(depthImageMemorys[i]);
  }

  for (auto framebuffer : swapChainFramebuffers) { device.device().destroyFramebuffer(framebuffer); }

  device.device().destroyRenderPass(renderPass);

  // cleanup synchronization objects
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
  {
    device.device().destroySemaphore(renderFinishedSemaphores[i]);
    device.device().destroySemaphore(imageAvailableSemaphores[i]);
    device.device().destroyFence(inFlightFences[i]);
  }
}

vk::Result HelloTriangleSwapChain::acquireNextImage(uint32_t* imageIndex)
{
  vk::Result wait_for_fences_result = device.device().waitForFences(1, &inFlightFences[currentFrame], true, std::numeric_limits<uint64_t>::max());
  if (wait_for_fences_result != vk::Result::eSuccess)
    throw std::runtime_error("Failed to wait for inFlightFences!");

  auto rv = device.device().acquireNextImageKHR(swapChain, std::numeric_limits<uint64_t>::max(),
      imageAvailableSemaphores[currentFrame],  // must be a not signaled semaphore
      {});

  *imageIndex = rv.value;
  return rv.result;
}

vk::Result HelloTriangleSwapChain::submitCommandBuffers(vk::CommandBuffer const* buffers, uint32_t* imageIndex)
{
  if (imagesInFlight[*imageIndex])
  {
    vk::Result wait_for_fences_result = device.device().waitForFences(1, &imagesInFlight[*imageIndex], true, std::numeric_limits<uint64_t>::max());
    if (wait_for_fences_result != vk::Result::eSuccess)
      throw std::runtime_error("Failed to wait for inFlightFences!");
  }
  imagesInFlight[*imageIndex] = inFlightFences[currentFrame];

  vk::SubmitInfo submitInfo;

  vk::Semaphore waitSemaphores[]      = { imageAvailableSemaphores[currentFrame] };
  vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
  submitInfo.waitSemaphoreCount     = 1;
  submitInfo.pWaitSemaphores        = waitSemaphores;
  submitInfo.pWaitDstStageMask      = waitStages;

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers    = buffers;

  vk::Semaphore signalSemaphores[]  = {renderFinishedSemaphores[currentFrame]};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores    = signalSemaphores;

  vk::Result reset_fences_result = device.device().resetFences(1, &inFlightFences[currentFrame]);
  if (reset_fences_result != vk::Result::eSuccess)
    throw std::runtime_error("Failed to reset fence for inFlightFences!");

  m_graphics_queue.submit(submitInfo, inFlightFences[currentFrame]);

  vk::PresentInfoKHR presentInfo;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores    = signalSemaphores;

  vk::SwapchainKHR swapChains[] = {swapChain};
  presentInfo.swapchainCount  = 1;
  presentInfo.pSwapchains     = swapChains;

  presentInfo.pImageIndices = imageIndex;

  auto result = m_present_queue.presentKHR(presentInfo);

  currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

  return result;
}

SwapChainSupportDetails getSwapChainSupport(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface)
{
  SwapChainSupportDetails details;

  details.capabilities = physical_device.getSurfaceCapabilitiesKHR(surface);
  details.formats = physical_device.getSurfaceFormatsKHR(surface);
  details.presentModes = physical_device.getSurfacePresentModesKHR(surface);

  return details;
}

struct QueueFamilyIndices
{
  uint32_t graphicsFamily;
  uint32_t presentFamily;
  bool graphicsFamilyHasValue = false;
  bool presentFamilyHasValue  = false;
  bool isComplete() { return graphicsFamilyHasValue && presentFamilyHasValue; }
};

QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device, vk::SurfaceKHR surface)
{
  QueueFamilyIndices indices;

  std::vector<vk::QueueFamilyProperties> queueFamilies;
  queueFamilies = device.getQueueFamilyProperties();

  int i = 0;
  for (auto const& queueFamily : queueFamilies)
  {
    if (queueFamily.queueCount > 0 && (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics))
    {
      indices.graphicsFamily         = i;
      indices.graphicsFamilyHasValue = true;
    }
    vk::Bool32 presentSupport = device.getSurfaceSupportKHR(i, surface);
    if (queueFamily.queueCount > 0 && presentSupport)
    {
      indices.presentFamily         = i;
      indices.presentFamilyHasValue = true;
    }
    if (indices.isComplete()) { break; }

    ++i;
  }

  return indices;
}

QueueFamilyIndices findPhysicalQueueFamilies(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface)
{
  return findQueueFamilies(physicalDevice, surface);
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

  swapChain = device.device().createSwapchainKHR(createInfo);
  swapChainImages = device.device().getSwapchainImagesKHR(swapChain);
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

    swapChainImageViews[i] = device.device().createImageView(viewInfo);
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

  renderPass = device.device().createRenderPass(renderPassInfo);
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

    swapChainFramebuffers[i] = device.device().createFramebuffer(framebufferInfo);
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
  image = device.device().createImage(imageInfo);

  vk::MemoryRequirements memRequirements;
  device.device().getImageMemoryRequirements(image, &memRequirements);

  vk::MemoryAllocateInfo allocInfo{};
  allocInfo.allocationSize  = memRequirements.size;
  allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties, device.get_physical_device());

  imageMemory = device.device().allocateMemory(allocInfo);

  device.device().bindImageMemory(image, imageMemory, 0);
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

    depthImageViews[i] = device.device().createImageView(viewInfo);
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
    imageAvailableSemaphores[i] = device.device().createSemaphore(semaphoreInfo);
    renderFinishedSemaphores[i] = device.device().createSemaphore(semaphoreInfo);
    inFlightFences[i] = device.device().createFence(fenceInfo);
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
