#include "sys.h"
#include "UniformBuffer.h"
#include "SynchronousWindow.h"

namespace vulkan::shader_builder::shader_resource {

void UniformBufferBase::create(task::SynchronousWindow const* window, vk::DeviceSize size
    COMMA_CWDEBUG_ONLY(Ambifix const& ambifix))
{
  DoutEntering(dc::vulkan, "UniformBufferBase::create(" << window << ", " << size << ")");
  for (vulkan::FrameResourceIndex i{0}; i != window->max_number_of_frame_resources(); ++i)
    m_uniform_buffers.emplace_back(window->logical_device(), size
      COMMA_CWDEBUG_ONLY(ambifix + "[" + to_string(i) + "]"));
}

std::string UniformBufferBase::glsl_id() const
{
  // Uniform buffers must contain a struct with at least one MEMBER.
  ASSERT(!m_members.empty());
  ShaderResourceMember const& member = m_members.begin()->second;
  return member.prefix();
}

#ifdef CWDEBUG
void UniformBufferBase::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_descriptor_set_key:" << m_descriptor_set_key <<
      ", m_members:{";
  char const* prefix = "";
  for (auto&& p : m_members)
  {
    os << prefix << '{' << p.first << ", " << p.second << '}';
    prefix = ", ";
  }
  os << "}, m_uniform_buffers:" << m_uniform_buffers;
  os << '}';
}
#endif

} // namespace vulkan::shader_builder::shader_resource
