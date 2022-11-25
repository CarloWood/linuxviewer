#include "sys.h"
#include "FactoryCharacteristicData.h"
#include "debug.h"

namespace vulkan::pipeline {

#ifdef CWDEBUG
void FactoryCharacteristicData::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_descriptor_set:" << m_descriptor_set <<
      ", m_binding:" << m_binding;
  os << '}';
}
#endif

} // namespace vulkan::pipeline
