#pragma once

#include <vulkan/vulkan.hpp>
#include <array>
#include <vector>
#include "debug.h"

#if defined(CWDEBUG) && !defined(DOXYGEN)
NAMESPACE_DEBUG_CHANNELS_START
extern channel_ct vulkan;
NAMESPACE_DEBUG_CHANNELS_END
#endif

namespace vulkan {

class LvInstance
{
 public:
#ifdef CWDEBUG
  static constexpr bool s_enableValidationLayers = true;
  static constexpr std::array<char const* const, 1> validationLayers = { "VK_LAYER_KHRONOS_validation" };
#else
  static constexpr bool s_enableValidationLayers = false;
  static constexpr std::array<char const* const, 0> validationLayers;
#endif

  static std::vector<char const*> getRequiredExtensions();

 protected:
  VkInstance m_instance;        // Per application state. Creating a VkInstance object initializes
                                // the Vulkan library and allows the application to pass information
                                // about itself to the implementation.
  ~LvInstance();

  void createInstance();

 private:
  bool checkValidationLayerSupport();
  void hasGflwRequiredInstanceExtensions(std::vector<char const*> const& requiredExtensions);
};

} // namespace vulkan
