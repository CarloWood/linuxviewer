#include "sys.h"
#include "SetKey.h"
#include "SetKeyContext.h"
#include "SynchronousWindow.h"
#include "debug.h"

namespace vulkan::descriptor {

//static
utils::UniqueIDContext<size_t> SetKey::s_dummy_context;

SetKey::SetKey(SetKeyContext& descriptor_set_key_context) :
  m_id(descriptor_set_key_context.get_id({})) COMMA_CWDEBUG_ONLY(m_debug_id_is_set(true))
{
}

#ifdef CWDEBUG
void SetKey::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_id:";
  if (m_debug_id_is_set)
    os << m_id;
  else
    os << "<uninitialized>";
  os << '}';
}
#endif

} // namespace vulkan::descriptor
