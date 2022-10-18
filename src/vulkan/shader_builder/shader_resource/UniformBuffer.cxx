#include "sys.h"
#include "UniformBuffer.h"
#include "SynchronousWindow.h"

namespace vulkan::shader_builder::shader_resource {

void UniformBufferBase::create2(task::SynchronousWindow const* owning_window
    COMMA_CWDEBUG_ONLY(Ambifix const& ambifix))
{
  DoutEntering(dc::shaderresource|dc::vulkan, "UniformBufferBase::create(" << owning_window << ")");
  // You must use at least 2 frame resources.
  ASSERT(owning_window->max_number_of_frame_resources().get_value() > 1);
  for (vulkan::FrameResourceIndex i{0}; i != owning_window->max_number_of_frame_resources(); ++i)
  {
    m_uniform_buffers.emplace_back(owning_window->logical_device(), size()
      COMMA_CWDEBUG_ONLY(".m_uniform_buffers[" + to_string(i) + "]" + ambifix));
  }
}

void UniformBufferBase::update_descriptor_set(task::SynchronousWindow const* owning_window, descriptor::FrameResourceCapableDescriptorSet const& descriptor_set, uint32_t binding, bool CWDEBUG_ONLY(has_frame_resource)) const
{
  DoutEntering(dc::shaderresource, "UniformBufferBase::update_descriptor_set(" << owning_window << ", " << descriptor_set << ", " << binding << ", " << std::boolalpha << has_frame_resource << ")");
  for (FrameResourceIndex frame_index{0}; frame_index < owning_window->max_number_of_frame_resources(); ++frame_index)
  {
    // Information about the buffer we want to point at in the descriptor.
    std::array<vk::DescriptorBufferInfo, 1> buffer_infos = {{
      m_uniform_buffers[frame_index].m_vh_buffer, 0, size()
    }};
    owning_window->logical_device()->update_descriptor_sets(descriptor_set[frame_index], vk::DescriptorType::eUniformBuffer, binding, 0 /*array_element*/, buffer_infos);
  }
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
  os << "(Base)";
  Base::print_on(os);
  os << ", m_members:" << m_members <<
        ", m_uniform_buffers:" << m_uniform_buffers;
  os << '}';
}
#endif

} // namespace vulkan::shader_builder::shader_resource
