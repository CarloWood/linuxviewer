#include "sys.h"
#include "QueueFlags.h"

namespace vulkan {

#ifdef CWDEBUG
void QueueFlags::print_on(std::ostream& os) const
{
  static constexpr auto& entries = magic_enum::enum_entries<QueueFlagBits>();
  char const* prefix = "";
  for (auto&& entry : entries)
    if (!!operator&(entry.first))
    {
      os << prefix;
      prefix = "|";
      os << entry.second;
    }
}
#endif

} // namespace vulkan
