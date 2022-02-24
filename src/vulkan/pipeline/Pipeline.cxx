#include "sys.h"
#include "Pipeline.h"
#include "SynchronousWindow.h"
#include "debug.h"

namespace vulkan::pipeline {

void Pipeline::generate(task::SynchronousWindow* owning_window COMMA_CWDEBUG_ONLY(AmbifixOwner const& ambifix))
{
  DoutEntering(dc::vulkan, "Pipeline::generate(" << owning_window << ") [" << this << "]");

  for (VertexShaderInputSetBase* vertex_shader_input_set : m_vertex_shader_input_sets)
  {
    size_t entry_size = vertex_shader_input_set->size();
    int count = vertex_shader_input_set->count();
    size_t buffer_size = count * entry_size;

    // FIXME: write directly into allocated vulkan buffer?
    std::vector<std::byte> buf(buffer_size);
    std::byte const* const end = buf.data() + buffer_size;
    int batch_size;
    for (std::byte* ptr = buf.data(); ptr != end; ptr += batch_size * entry_size)
    {
      batch_size = vertex_shader_input_set->next_batch();
      vertex_shader_input_set->get_input_entry(ptr);
    }

    // FIXME: do we really have to keep this object around?
    m_buffers.emplace_back(owning_window->logical_device().create_buffer(buffer_size,
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal
        COMMA_CWDEBUG_ONLY(ambifix("m_vertex_buffer"))));

    m_vhv_buffers.push_back(*m_buffers.back().m_buffer);

    owning_window->copy_data_to_buffer(buffer_size, buf.data(), *m_buffers.back().m_buffer, 0, vk::AccessFlags(0),
        vk::PipelineStageFlagBits::eTopOfPipe, vk::AccessFlagBits::eVertexAttributeRead, vk::PipelineStageFlagBits::eVertexInput);
  }
}

} // namespace vulkan::pipeline
