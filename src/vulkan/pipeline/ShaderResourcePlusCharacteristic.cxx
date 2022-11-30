#include "sys.h"
#include "ShaderResourcePlusCharacteristic.h"
#include "pipeline/CharacteristicRange.h"
#include "vk_utils/print_pointer.h"
#include "debug.h"

namespace vulkan::pipeline {

bool ShaderResourcePlusCharacteristic::has_unbounded_descriptor_array_size() const
{
  //FIXME: Shouldn't we pass m_fill_index here and make it so that array sizes (and unbounded-ness) can be a function of that?
  return m_shader_resource->descriptor_array_size() < 0;
}

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

} // namespace vulkan::pipeline
