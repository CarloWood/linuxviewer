#pragma once

#include "Device.h"

// vulkan headers
#include <vulkan/vulkan.h>

// std lib headers
#include <string>
#include <vector>

namespace vulkan {

class HelloTriangleSwapChain {
 public:
  static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

  HelloTriangleSwapChain(Device const& deviceRef);
  ~HelloTriangleSwapChain();

  HelloTriangleSwapChain(const HelloTriangleSwapChain &) = delete;
  void operator=(const HelloTriangleSwapChain &) = delete;

  void setup(VkExtent2D windowExtent, vk::Queue graphics_queue, vk::Queue present_queue, VkSurfaceKHR surface);

  VkFramebuffer getFrameBuffer(int index) const { return swapChainFramebuffers[index]; }
  VkRenderPass getRenderPass() const { return renderPass; }
  VkImageView getImageView(int index) { return swapChainImageViews[index]; }
  size_t imageCount() const { return swapChainImages.size(); }
  VkFormat getSwapChainImageFormat() const { return swapChainImageFormat; }
  VkExtent2D getSwapChainExtent() const { return swapChainExtent; }
  uint32_t width() const { return swapChainExtent.width; }
  uint32_t height() const { return swapChainExtent.height; }

  float extentAspectRatio() {
    return static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height);
  }
  VkFormat findDepthFormat();

  VkResult acquireNextImage(uint32_t *imageIndex);
  VkResult submitCommandBuffers(const VkCommandBuffer *buffers, uint32_t *imageIndex);

 private:
  void createSwapChain(VkSurfaceKHR surface);
  void createImageViews();
  void createDepthResources();
  void createRenderPass();
  void createFramebuffers();
  void createSyncObjects();

  // Helper functions
  VkSurfaceFormatKHR chooseSwapSurfaceFormat(
      const std::vector<VkSurfaceFormatKHR> &availableFormats);
  VkPresentModeKHR chooseSwapPresentMode(
      const std::vector<VkPresentModeKHR> &availablePresentModes);
  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

  VkFormat swapChainImageFormat;
  VkExtent2D swapChainExtent;

  std::vector<VkFramebuffer> swapChainFramebuffers;
  VkRenderPass renderPass;

  std::vector<VkImage> depthImages;
  std::vector<VkDeviceMemory> depthImageMemorys;
  std::vector<VkImageView> depthImageViews;
  std::vector<VkImage> swapChainImages;
  std::vector<VkImageView> swapChainImageViews;

  Device const& device;
  vk::Queue m_graphics_queue;
  vk::Queue m_present_queue;
  VkExtent2D windowExtent;

  VkSwapchainKHR swapChain;

  std::vector<VkSemaphore> imageAvailableSemaphores;
  std::vector<VkSemaphore> renderFinishedSemaphores;
  std::vector<VkFence> inFlightFences;
  std::vector<VkFence> imagesInFlight;
  size_t currentFrame = 0;
};

} // namespace vulkan
