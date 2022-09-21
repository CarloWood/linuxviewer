#include "sys.h"
#ifdef CWDEBUG
#include "SetBindingMap.h"
#endif
#include "debug.h"

namespace vulkan::descriptor {

#ifdef CWDEBUG
void SetBindingMap::print_on(std::ostream& os) const
{
  os << '{';
  char const* prefix = "";
  for (SetIndexHint set_index_hint = m_set_map.ibegin(); set_index_hint != m_set_map.iend(); ++set_index_hint)
  {
    SetData const& set_data = m_set_map[set_index_hint];
    for (uint32_t binding = 0; binding != set_data.m_binding_map.size(); ++binding)
    {
      os << prefix << '{' <<
        set_index_hint.get_value() << '.' << binding << " --> " <<
        set_data.m_set_index.get_value() << '.' << set_data.m_binding_map[binding] << '}';
      prefix = ", ";
    }
  }
  os << '}';
}
#endif

} // namespace vulkan::descriptor
