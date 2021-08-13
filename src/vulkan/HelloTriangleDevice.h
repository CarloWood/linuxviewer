#pragma once

#include <vulkan/vulkan.hpp>    // Must be included BEFORE glfwpp/glfwpp.h in order to get vulkan C++ API support.
#include <glfwpp/glfwpp.h>      // For glfw::Window
#include "debug.h"
#include <string>
#include <vector>

namespace vulkan {

class HelloTriangleDevice
{
 public:
  HelloTriangleDevice() = default;
  ~HelloTriangleDevice();

  void setup(VkInstance instance, VkSurfaceKHR surface);

  // Not copyable or movable
  HelloTriangleDevice(HelloTriangleDevice const&) = delete;
  void operator=(HelloTriangleDevice const&) = delete;
  HelloTriangleDevice(HelloTriangleDevice&&) = delete;
  HelloTriangleDevice& operator=(HelloTriangleDevice&&) = delete;

  VkCommandPool getCommandPool() const { return commandPool; }
  vk::Device device() const { return device_; }
  VkSurfaceKHR surface() const { return surface_; }
  VkQueue graphicsQueue() const { return graphicsQueue_; }
  VkQueue presentQueue() const { return presentQueue_; }

  uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
  VkFormat findSupportedFormat(std::vector<VkFormat> const& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

  // Buffer Helper Functions
  void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
  VkCommandBuffer beginSingleTimeCommands();
  void endSingleTimeCommands(VkCommandBuffer commandBuffer);
  void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
  void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount);

  void createImageWithInfo(VkImageCreateInfo const& imageInfo, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);

  VkPhysicalDeviceProperties properties;

 private:
  void pickPhysicalDevice();
  void createLogicalDevice();
  void createCommandPool();

  // helper functions
  bool isDeviceSuitable(VkPhysicalDevice device);
  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
  bool checkDeviceExtensionSupport(VkPhysicalDevice device);
  SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

  vk::PhysicalDevice physicalDevice = {};
  VkCommandPool commandPool;

  VkInstance instance_;         // FIXME: this should not be here.
  VkSurfaceKHR surface_;        // FIXME: this should not be here.
  vk::Device device_;
  VkQueue graphicsQueue_;
  VkQueue presentQueue_;

  std::vector<char const*> const deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

 public:
#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

}  // namespace vulkan
