#include "sys.h"
#include "PushConstantRange.h"
#include "vk_utils/print_flags.h"

namespace vulkan {

// The order in which PushConstantRange objects are sorted is described in FlatCreateInfo::get_sorted_push_constant_ranges.
bool operator<(PushConstantRange const& lhs, PushConstantRange const& rhs)
{
  if (lhs.m_type_index != rhs.m_type_index)
    return lhs.m_type_index < rhs.m_type_index;

  if (lhs.m_shader_stage_flags != rhs.m_shader_stage_flags)
    return lhs.m_shader_stage_flags < rhs.m_shader_stage_flags;

  if (lhs.m_offset != rhs.m_offset)
    return lhs.m_offset < rhs.m_offset;

  // The size is compared too, because we don't want to remove elements that are the same but have different size,
  // when running std::set_union on them in FlatCreateInfo::get_sorted_push_constant_ranges.
  return lhs.m_size < rhs.m_size;
}

#ifdef CWDEBUG
void PushConstantRange::print_on(std::ostream& os) const
{
  os << '{';
  // This seems to work on g++...
  std::type_info* ptr;
  std::memcpy(&ptr, &m_type_index, sizeof(ptr));
  os << "m_type_index:" << ptr->name() <<
      ", m_shader_stage_flags:" << m_shader_stage_flags <<
      ", m_offset:" << m_offset <<
      ", m_size:" << m_size;
  os << '}';
}
#endif

} // namespace vulkan
