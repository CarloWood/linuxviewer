#include "sys.h"
#include "FactoryCharacteristicKey.h"
#include "debug.h"

namespace vulkan::pipeline {

#ifdef CWDEBUG
void FactoryCharacteristicKey::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_id:" << m_id <<
      ", m_subrange:" << m_subrange;
  os << '}';
}
#endif

} // namespace vulkan::pipeline
