#pragma once

#include "CommandBuffer.h"
#include "memory/Buffer.h"
#include <vector>
#ifdef CWDEBUG
#include <iosfwd>
#endif

namespace vulkan {
class SynchronousWindow;

namespace shader_builder {
class VertexShaderInputSetBase;
} // namespace shader_builder

class VertexBuffers
{
 private:
  std::vector<memory::Buffer> m_memory;                 // The actual buffers and their memory allocation.
                                                        //
  uint32_t m_binding_count;                             // Cache value to be used by bind().
  void* m_pointers;                                     // Idem.

 public:
  VertexBuffers() : m_binding_count(0), m_pointers(nullptr) { }

  void create_vertex_buffer(
      SynchronousWindow const* owning_window,
      shader_builder::VertexShaderInputSetBase const& vertex_shader_input_set);

  void bind(handle::CommandBuffer command_buffer)
  {
    command_buffer->bindVertexBuffers(0 /* uint32_t first_binding */,
        m_binding_count,
        static_cast<vk::Buffer*>(m_pointers),
        static_cast<vk::DeviceSize*>(m_pointers + m_binding_count));
  }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan
