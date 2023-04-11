#pragma once

#include "../Defaults.h"
#include "../Application.h"

namespace vulkan {

class DebugUtilsMessengerCreateInfoEXT : public vk_defaults::DebugUtilsMessengerCreateInfoEXT
{
 public:
  DebugUtilsMessengerCreateInfoEXT(PFN_vkDebugUtilsMessengerCallbackEXT user_call_back, void* user_data);
};

} // namespace vulkan
