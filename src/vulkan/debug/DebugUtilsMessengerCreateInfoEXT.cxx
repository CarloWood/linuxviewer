#include "sys.h"
#include "DebugUtilsMessengerCreateInfoEXT.h"

namespace vulkan {

DebugUtilsMessengerCreateInfoEXT::DebugUtilsMessengerCreateInfoEXT(vk::PFN_DebugUtilsMessengerCallbackEXT user_call_back, void* user_data)
{
  if (pfnUserCallback == nullptr && pUserData == nullptr)
  {
    pfnUserCallback = user_call_back;
    pUserData = user_data;
  }
}

} // namespace vulkan
