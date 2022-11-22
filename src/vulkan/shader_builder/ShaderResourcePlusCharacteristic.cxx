#include "sys.h"
#include "ShaderResourcePlusCharacteristic.h"
#include "pipeline/CharacteristicRange.h"
#include "vk_utils/print_pointer.h"
#include "debug.h"

namespace vulkan::shader_builder {

#ifdef CWDEBUG
void ShaderResourcePlusCharacteristic::print_on(std::ostream& os) const
{
  using namespace vk_utils;
  os << '{';
  os << "m_shader_resource:" << print_pointer(m_shader_resource) <<
      ", m_characteristic_range:" << print_pointer(m_characteristic_range) <<
      ", m_fill_index:" << m_fill_index;
  os << '}';
}
#endif

} // namespace vulkan::shader_builder
