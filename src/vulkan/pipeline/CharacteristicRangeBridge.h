#pragma once

#include "VertexBufferBindingIndex.h"
#include "descriptor/SetIndex.h"
#include "utils/Vector.h"
#include <vulkan/vulkan.hpp>
#include "debug.h"

namespace vulkan::task {
class PipelineFactory;
} // namespace vulkan::task

namespace vulkan::descriptor {
class SetIndexHintMap;
} // namespace vulkan::descriptor

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
  virtual utils::Vector<shader_builder::VertexShaderInputSetBase*, VertexBufferBindingIndex> const& vertex_shader_input_sets() const
  {
    ASSERT(false);
    AI_NEVER_REACHED
  }

  // Implemented by AddShaderStage.
  virtual bool is_add_shader_stage() const { return false; }
  virtual void register_AddShaderStage_with(task::PipelineFactory* pipeline_factory) const { }
  virtual void set_set_index_hint_map(descriptor::SetIndexHintMap const* set_index_hint_map)
  {
    ASSERT(false);
    AI_NEVER_REACHED
  }
  virtual void preprocess_shaders_and_realize_descriptor_set_layouts(task::PipelineFactory* pipeline_factory)
  {
    ASSERT(false);
    AI_NEVER_REACHED
  }
  virtual void build_shaders(task::PipelineFactory* pipeline_factory)
  {
    ASSERT(false);
    AI_NEVER_REACHED
  }
  //virtual void destroy_shader_module_handles() { }

  // Implemented by AddVertexShader.
  virtual void copy_shader_variables() { }
  virtual void update_vertex_input_descriptions() { }
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
