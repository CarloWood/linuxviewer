#include "sys.h"
#include "FactoryCharacteristicId.h"
#include "debug.h"

namespace vulkan::pipeline {

#ifdef CWDEBUG
void FactoryCharacteristicId::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_factory_index:" << m_factory_index <<
      ", m_range_index:" << m_range_index <<
      ", m_characteristic_range_size:" << m_characteristic_range_size;
  os << '}';
}
#endif

} // namespace vulkan::pipeline
