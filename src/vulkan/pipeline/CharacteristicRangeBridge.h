#pragma once

#include "descriptor/SetIndex.h"
#include "utils/Vector.h"
#include <vulkan/vulkan.hpp>
#include "debug.h"

namespace task {
class PipelineFactory;
} // namespace task

namespace vulkan::shader_builder {
class VertexShaderInputSetBase;
class ShaderResourceDeclaration;
class ShaderResourceBase;
} // namespace vulkan::shader_builder

namespace vulkan::pipeline {

// This bridges the CharacteristicRange to other base classes that are used for a Characteristic user class.
struct CharacteristicRangeBridge
{
  // Make the destructor virtual even though we never delete by CharacteristicRangeBridge*.
  virtual ~CharacteristicRangeBridge() = default;

  // Implemented by AddVertexShader.
  virtual utils::Vector<shader_builder::VertexShaderInputSetBase*> const& vertex_shader_input_sets() const { ASSERT(false); AI_NEVER_REACHED }

  // Implemented by AddShaderStage.
  virtual bool is_add_shader_stage() const { return false; }
  virtual void register_AddShaderStage_with(task::PipelineFactory* pipeline_factory) const { }

  // Implemented by AddVertexShader.
  virtual void copy_vertex_input_binding_descriptions() { }
  virtual void copy_vertex_input_attribute_descriptions() { }
  virtual void register_AddVertexShader_with(task::PipelineFactory* pipeline_factory) const { }

  // Implemented by AddFragmentShader.
  virtual void register_AddFragmentShader_with(task::PipelineFactory* pipeline_factory) const { }

  // Implemented by CharacteristicRange.
  virtual shader_builder::ShaderResourceDeclaration* realize_shader_resource_declaration(std::string glsl_id_full, vk::DescriptorType descriptor_type, shader_builder::ShaderResourceBase const& shader_resource, descriptor::SetIndexHint set_index_hint) = 0;

  virtual task::PipelineFactory* get_owning_factory() const = 0;

  // Implemented by AddPushConstant.
  virtual void copy_push_constant_ranges(task::PipelineFactory* pipeline_factory) { }
  virtual void register_AddPushConstant_with(task::PipelineFactory* pipeline_factory) const { }
};

} // namespace vulkan::pipeline
