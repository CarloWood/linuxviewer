#pragma once

#include "VertexShaderInputSet.h"
#include "debug/DebugSetName.h"
#include <vector>

namespace task {
class SynchronousWindow;
} // namespace task

namespace vulkan::pipeline {

class Pipeline
{
 private:
  std::vector<VertexShaderInputSetBase*> m_vertex_shader_input_sets;
  std::vector<BufferParameters> m_buffers;
  std::vector<vk::Buffer> m_vhv_buffers;

 public:
  void bind(VertexShaderInputSetBase& vertex_shader_input_set)
  {
    // Keep track of all VertexShaderInputSetBase objects.
    m_vertex_shader_input_sets.push_back(&vertex_shader_input_set);
  }

  void generate(task::SynchronousWindow* owning_window
       COMMA_CWDEBUG_ONLY(AmbifixOwner const& ambifix));

  std::vector<vk::Buffer> const& vhv_buffers() const
  {
    return m_vhv_buffers;
  }
};

} // namespace vulkan::pipeline
