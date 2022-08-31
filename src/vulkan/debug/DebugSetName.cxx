#include "sys.h"
#include "DebugSetName.h"
#include "SynchronousWindow.h"
#include "utils/ulong_to_base.h"

namespace vulkan {

// Create a postfix for AmbifixOwner of the form " [0xhexaddress]", to be used as postfix.
std::string as_postfix(void const* ptr, char const* open_bracket, char const* close_bracket)
{
  std::string result = " ";
  result += open_bracket;
  result += "0x";
  result += utils::ulong_to_base(reinterpret_cast<uint64_t>(ptr), "0123456789abcdef");
  result += close_bracket;
  return result;
}

AmbifixOwner::AmbifixOwner(task::SynchronousWindow const* owning_window, std::string prefix) :
  Ambifix(std::move(prefix), as_postfix(owning_window)),
  m_logical_device(owning_window->logical_device()) { }

AmbifixOwner::AmbifixOwner(vulkan::LogicalDevice const* owning_logical_device, std::string prefix) :
  Ambifix(std::move(prefix), as_postfix(owning_logical_device, "(", ")")),
  m_logical_device(owning_logical_device) { }

} // namespace vulkan
