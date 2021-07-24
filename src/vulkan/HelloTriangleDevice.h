#pragma once

#include <glfwpp/glfwpp.h>  // For glfw::Window
#include "LvInstance.h"
#include "debug.h"

// std lib headers
#include <string>
#include <vector>

namespace vulkan {

struct SwapChainSupportDetails
{
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilyIndices
{
  uint32_t graphicsFamily;
  uint32_t presentFamily;
  bool graphicsFamilyHasValue = false;
  bool presentFamilyHasValue  = false;
  bool isComplete() { return graphicsFamilyHasValue && presentFamilyHasValue; }
};

class HelloTriangleDevice : public LvInstance
{
 public:
  HelloTriangleDevice(glfw::Window& window);
  ~HelloTriangleDevice();

  // Not copyable or movable
  HelloTriangleDevice(HelloTriangleDevice const&) = delete;
  void operator=(HelloTriangleDevice const&) = delete;
  HelloTriangleDevice(HelloTriangleDevice&&) = delete;
  HelloTriangleDevice& operator=(HelloTriangleDevice&&) = delete;

  VkCommandPool getCommandPool() const { return commandPool; }
  VkDevice device() const { return device_; }
  VkSurfaceKHR surface() const { return surface_; }
  VkQueue graphicsQueue() const { return graphicsQueue_; }
  VkQueue presentQueue() const { return presentQueue_; }

  SwapChainSupportDetails getSwapChainSupport() { return querySwapChainSupport(physicalDevice); }
  uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
  QueueFamilyIndices findPhysicalQueueFamilies() { return findQueueFamilies(physicalDevice); }
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
  void setupDebugMessenger();
  void createSurface();
  void pickPhysicalDevice();
  void createLogicalDevice();
  void createCommandPool();

  // helper functions
  bool isDeviceSuitable(VkPhysicalDevice device);
  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
  void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
  bool checkDeviceExtensionSupport(VkPhysicalDevice device);
  SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

  VkDebugUtilsMessengerEXT debugMessenger;
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  glfw::Window& m_window;
  VkCommandPool commandPool;

  VkDevice device_;
  VkSurfaceKHR surface_;
  VkQueue graphicsQueue_;
  VkQueue presentQueue_;

  std::vector<char const*> const deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

 public:
#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

}  // namespace vulkan
