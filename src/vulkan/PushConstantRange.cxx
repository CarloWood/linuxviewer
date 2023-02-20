#include "sys.h"

#ifdef CWDEBUG
// This must be included as one of the first header files.
#include "debug/xml_serialize.h"
#include "xml/Bridge.h"
#endif

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

void PushConstantRange::xml(xml::Bridge& xml)
{
  xml.node_name("push_constant_range");

  xml.child(m_type_index);
  xml.child_stream("offset", m_offset);
  xml.child_stream("size", m_size);

  vk::ShaderStageFlags flags{m_shader_stage_flags.load(std::memory_order::relaxed)};
  xml.child_stream("flags", flags);

  if (!xml.writing())
  {
    m_shader_stage_flags.store(static_cast<vk::ShaderStageFlags::MaskType>(flags), std::memory_order::relaxed);
  }
}
#endif

} // namespace vulkan

//  mutable std::atomic<vk::ShaderStageFlags::MaskType> m_shader_stage_flags;     // The shader stages that use this range.

#ifdef CWDEBUG
namespace xml {

template<>
void serialize(std::type_index& type_index, xml::Bridge& xml)
{
  xml.node_name("type_index");

  if (xml.writing())
  {
    std::string name = type_index.name();
    xml.text_stream(name);
  }
}

template
void serialize(std::type_index& type_index, xml::Bridge& xml);

} // namespace xml

namespace vulkan {

} // namespace vulkan

#endif // CWDEBUG
