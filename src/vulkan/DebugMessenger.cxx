#include "sys.h"

#ifdef CWDEBUG
#include "DebugMessenger.h"

namespace vulkan {

void DebugMessenger::setup(vk::Instance vh_instance, vk::DebugUtilsMessengerCreateInfoEXT const& debug_create_info)
{
  // Keep a copy, because we need that in the destructor.
  m_vh_instance = vh_instance;
  m_debug_messenger = m_vh_instance.createDebugUtilsMessengerEXTUnique(debug_create_info);
}

} // namespace vulkan
#endif
