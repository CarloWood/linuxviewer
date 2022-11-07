#include "sys.h"
#ifdef CWDEBUG
#include "SetIndexHintMap.h"
#endif
#include "debug.h"

namespace vulkan::descriptor {

#ifdef CWDEBUG
void SetIndexHintMap::print_on(std::ostream& os) const
{
  os << '{';
  char const* prefix = "";
  for (SetIndexHint set_index_hint = m_set_index_map.ibegin(); set_index_hint != m_set_index_map.iend(); ++set_index_hint)
  {
    SetIndex set_index = m_set_index_map[set_index_hint];
    os << prefix << '{' << set_index_hint << " --> " << set_index << '}';
    prefix = ", ";
  }
  os << '}';
}
#endif

} // namespace vulkan::descriptor
