#include "sys.h"
#include "VertexBuffers.h"
#include "SynchronousWindow.h"
#include "queues/CopyDataToBuffer.h"
#include "shader_builder/VertexShaderInputSet.h"
#include "debug.h"

namespace vulkan {

void VertexBuffers::create_vertex_buffer(
    SynchronousWindow const* owning_window,
    shader_builder::VertexShaderInputSetBase const& vertex_shader_input_set)
{
  DoutEntering(dc::vulkan, "VertexBuffers::create_vertex_buffer(" << owning_window << ", " << vertex_shader_input_set << ") [" << this << "]");

  size_t entry_size = vertex_shader_input_set.chunk_size();
  int count = vertex_shader_input_set.chunk_count();
  size_t buffer_size = count * entry_size;

  m_memory.push_back(memory::Buffer{owning_window->logical_device(), buffer_size,
      { .usage = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
        .properties = vk::MemoryPropertyFlagBits::eDeviceLocal }
      COMMA_CWDEBUG_ONLY(owning_window->debug_name_prefix("m_memory[" + std::to_string(m_memory.size()) + "]"))});
  vk::Buffer new_buffer = m_memory.back().m_vh_buffer;

  // The memory layout of the m_pointers memory block is:
  // vk::Buffer*
  //   .
  //   .
  // vk::Buffer*
  // vk::DeviceSize*    <-- buffer_end
  //   .
  //   .
  // vk::DeviceSize*
  // where there are m_binding_count vk::Buffer*'s as well as vk::DeviceSize*'s.
  void* buffer_end = m_pointers + m_binding_count;
  ++m_binding_count;
  m_pointers = std::realloc(2 * m_binding_count * sizeof(void*));
  // After the realloc we need to move the vk::DeviceSize*'s in memory one up to make place for the new vk::Buffer*.
  std::memmove(buffer_end + 1, buffer_end, m_binding_count - 1);
  *static_cast<vk::Buffer*>(buffer_end) = new_buffer;
  *static_cast<vk::DeviceSize*>(buffer_end + m_binding_count) = 0;  // All offsets are zero at the moment.

  auto copy_data_to_buffer = statefultask::create<task::CopyDataToBuffer>(owning_window->logical_device(), buffer_size, new_buffer, 0,
      vk::AccessFlags(0), vk::PipelineStageFlagBits::eTopOfPipe, vk::AccessFlagBits::eVertexAttributeRead,
      vk::PipelineStageFlagBits::eVertexInput COMMA_CWDEBUG_ONLY(true));

  copy_data_to_buffer->set_resource_owner(owning_window);       // Wait for this task to finish before destroying this window,
                                                                // because this window owns the buffer (m_vertex_buffers.back()),
                                                                // as well as vertex_shader_input_set (or at least, it should).
  copy_data_to_buffer->set_data_feeder(
      std::make_unique<shader_builder::VertexShaderInputSetFeeder>(&vertex_shader_input_set));

  copy_data_to_buffer->run(Application::instance().low_priority_queue());
}

#ifdef CWDEBUG
void VertexBuffers::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_memory:" << m_memory <<
      ", m_binding_count:" << m_binding_count <<
      ", m_pointers:" << m_pointers;
  os << '}';
}
#endif

} // namespace vulkan
