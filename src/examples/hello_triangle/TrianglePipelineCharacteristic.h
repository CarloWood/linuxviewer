#pragma once

#include "Window.h"
#include <vulkan/pipeline/Characteristic.h>
#include <vulkan/pipeline/AddVertexShader.h>
#include <vulkan/pipeline/AddFragmentShader.h>
#include "debug.h"

class TrianglePipelineCharacteristic :
    public vulkan::pipeline::Characteristic,
    public vulkan::pipeline::AddVertexShader,
    public vulkan::pipeline::AddFragmentShader
{
 private:
  std::vector<vk::DynamicState> m_dynamic_states = {
    vk::DynamicState::eViewport,
    vk::DynamicState::eScissor
  };

  std::vector<vk::PipelineColorBlendAttachmentState> m_pipeline_color_blend_attachment_states = {
    vk_defaults::PipelineColorBlendAttachmentState{}
  };

 public:
  TrianglePipelineCharacteristic(vulkan::task::SynchronousWindow const* owning_window COMMA_CWDEBUG_ONLY(bool debug)) :
    vulkan::pipeline::Characteristic(owning_window COMMA_CWDEBUG_ONLY(debug))
  {
    // Required when we don't have vertex buffers.
    m_use_vertex_buffers = false;
  }

 protected:
  void initialize() final
  {
    Window const* window = static_cast<Window const*>(m_owning_window);

    // Register the vectors that we will fill.
    add_to_flat_create_info(m_dynamic_states);
    add_to_flat_create_info(m_pipeline_color_blend_attachment_states);

    // Add default topology.
    m_flat_create_info->m_pipeline_input_assembly_state_create_info.topology = vk::PrimitiveTopology::eTriangleList;

    // Register the two shaders that we use.
    add_shader(window->m_shader_vert);
    add_shader(window->m_shader_frag);
  }

 public:
#ifdef CWDEBUG
  // Implement pure virtual function from the base class that allows us to write this class as debug output.
  void print_on(std::ostream& os) const override
  {
    // Just print the type, not any contents.
    os << "{ (FragmentPipelineCharacteristicRange*)" << this << " }";
  }
#endif
};
