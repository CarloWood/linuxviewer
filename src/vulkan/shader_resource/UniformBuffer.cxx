#include "sys.h"
#include "UniformBuffer.h"
#include "SynchronousWindow.h"

namespace vulkan::shader_resource {

void UniformBuffer::create(task::SynchronousWindow const* window, vk::DeviceSize size
    COMMA_CWDEBUG_ONLY(Ambifix const& ambifix))
{
  for (vulkan::FrameResourceIndex i{0}; i != window->max_number_of_frame_resources(); ++i)
    m_uniform_buffers.emplace_back(window->logical_device(), size
      COMMA_CWDEBUG_ONLY(ambifix + "[" + to_string(i) + "]"));
}

#ifdef CWDEBUG
void UniformBuffer::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_descriptor_set_key:" << m_descriptor_set_key <<
      ", m_uniform_buffers:" << m_uniform_buffers;
  os << '}';
}
#endif

} // namespace vulkan::shader_resource
