#pragma once

#include "CommandBuffer.h"
#include "VertexBufferBindingIndex.h"
#include "memory/Buffer.h"
#include <vector>
#ifdef CWDEBUG
#include <iosfwd>
#endif

namespace task {
class SynchronousWindow;
} // namespace task

namespace vulkan {
namespace shader_builder {
class VertexShaderInputSetBase;
} // namespace shader_builder

class VertexBuffers
{
 private:
  utils::Vector<memory::Buffer, VertexBufferBindingIndex> m_memory;     // The actual buffers and their memory allocation.
                                                                        //
  uint32_t m_binding_count;                                             // Cache value to be used by bind().
  char* m_data;                                                         // Idem.

 public:
  VertexBuffers() : m_binding_count(0), m_data(nullptr) { }

  void create_vertex_buffer(
      task::SynchronousWindow const* owning_window,
      shader_builder::VertexShaderInputSetBase& vertex_shader_input_set);

  void bind(handle::CommandBuffer command_buffer)
  {
    command_buffer->bindVertexBuffers(0 /* uint32_t first_binding */,
        m_binding_count,
        reinterpret_cast<vk::Buffer*>(m_data),
        reinterpret_cast<vk::DeviceSize*>(m_data + m_binding_count * sizeof(vk::Buffer)));
  }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan
