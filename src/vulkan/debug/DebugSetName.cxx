#include "sys.h"
#include "DebugSetName.h"
#include "SynchronousWindow.h"
#include "utils/ulong_to_base.h"

namespace vulkan {

std::string as_postfix(AIStatefulTask const* task)
{
  std::string result = " [0x";
  result += utils::ulong_to_base(reinterpret_cast<uint64_t>(task), "0123456789abcdef");
  result += ']';
  return result;
}

AmbifixOwner::AmbifixOwner(task::SynchronousWindow const* owning_window, std::string prefix) :
  Ambifix(std::move(prefix), as_postfix(owning_window)),
  m_owning_window(owning_window),
  m_logical_device(owning_window->logical_device()) { }

} // namespace vulkan
