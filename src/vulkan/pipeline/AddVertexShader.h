#pragma once

#include "shader_builder/VertexShaderInputSet.h"
#include "AddShaderStage.h"
#include "shader_builder/VertexAttribute.h"
#include "shader_builder/VertexAttributeDeclarationContext.h"
#include "utils/Vector.h"
#include "utils/Badge.h"
#include "utils/log2.h"
#include <vulkan/vulkan.hpp>
#include <vector>
#include "debug.h"

namespace vulkan {
class VertexBuffers;
namespace shader_builder {
class VertexShaderInputSetBase;
} // namespace shader_builder
} // namespace vulkan

namespace vulkan::pipeline {
class CharacteristicRange;

class AddVertexShader : public virtual AddShaderStage
{
 protected:
  VertexBuffers const* m_current_vertex_buffers_object{};
  std::vector<vk::VertexInputBindingDescription> m_vertex_input_binding_descriptions;
  std::vector<vk::VertexInputAttributeDescription> m_vertex_input_attribute_descriptions;
  bool m_use_vertex_buffers{true};

  //---------------------------------------------------------------------------
  // Vertex attributes.
  shader_builder::VertexAttributeDeclarationContext m_vertex_shader_location_context;   // Location context used for vertex attributes
                                                                                        // (VertexAttribute).
 public:
  // Implementation of virtual functions of AddShaderStage.
  void copy_shader_variables() final;
  void update_vertex_input_descriptions() final;
  void register_AddVertexShader_with(task::PipelineFactory* pipeline_factory) const final;

 public:
  AddVertexShader() = default;

  // Called from VertexAttribute::is_used_in.
  shader_builder::VertexAttributeDeclarationContext* vertex_shader_location_context(utils::Badge<shader_builder::VertexAttribute>) override
  {
    return &m_vertex_shader_location_context;
  }

 protected:
  void add_vertex_input_bindings(VertexBuffers const& vertex_buffers);

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::pipeline
