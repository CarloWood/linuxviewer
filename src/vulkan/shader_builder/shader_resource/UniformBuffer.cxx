#include "sys.h"
#include "UniformBuffer.h"
#include "SynchronousWindow.h"

namespace vulkan::shader_builder::shader_resource {

void UniformBufferBase::create(task::SynchronousWindow const* owning_window)
{
  DoutEntering(dc::shaderresource|dc::vulkan, "UniformBufferBase::create(" << owning_window << ")");
  for (vulkan::FrameResourceIndex i{0}; i != owning_window->max_number_of_frame_resources(); ++i)
  {
    m_uniform_buffers.emplace_back(owning_window->logical_device(), size()
      COMMA_CWDEBUG_ONLY(ambifix() + "[" + to_string(i) + "]"));
  }
}

void UniformBufferBase::update_descriptor_set(task::SynchronousWindow const* owning_window, vk::DescriptorSet vh_descriptor_set, uint32_t binding)
{
  // FIXME: not implemented: currently we're only using the first frame resource!
  vulkan::FrameResourceIndex const hack0{0};

  // Information about the buffer we want to point at in the descriptor.
  std::vector<vk::DescriptorBufferInfo> buffer_infos = {
    {
      .buffer = m_uniform_buffers[hack0].m_vh_buffer,
      .offset = 0,
      .range = size()
    }
  };
  owning_window->logical_device()->update_descriptor_set(vh_descriptor_set, vk::DescriptorType::eUniformBuffer, binding, 0, {}, buffer_infos);
  //create_uniform_buffers
}

std::string UniformBufferBase::glsl_id() const
{
  // Uniform buffers must contain a struct with at least one MEMBER.
  ASSERT(!m_members.empty());
  ShaderResourceMember const& member = *m_members.begin();
  return member.prefix();
}

#ifdef CWDEBUG
void UniformBufferBase::print_on(std::ostream& os) const
{
  os << '{';
  os << "Base::m_descriptor_set_key:" << descriptor_set_key() <<
      ", m_members:" << m_members <<
      ", m_uniform_buffers:" << m_uniform_buffers;
  os << '}';
}
#endif

} // namespace vulkan::shader_builder::shader_resource
