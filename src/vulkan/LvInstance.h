#pragma once

#include <vulkan/vulkan.hpp>
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
#else
  static constexpr bool s_enableValidationLayers = false;
#endif

 protected:
  VkInstance m_instance;        // Per application state. Creating a VkInstance object initializes
                                // the Vulkan library and allows the application to pass information
                                // about itself to the implementation.
  ~LvInstance();

  void createInstance();

 private:
  bool checkValidationLayerSupport();
  std::vector<char const*> getRequiredExtensions();
  void hasGflwRequiredInstanceExtensions(std::vector<char const*> const& requiredExtensions);

 private:
  std::vector<char const*> const validationLayers = {"VK_LAYER_KHRONOS_validation"};
};

} // namespace vulkan
