#pragma once

#include "VertexBufferBindingIndex.h"
#include "descriptor/SetIndex.h"
#include "statefultask/AIStatefulTask.h"
#include "utils/Vector.h"
#include <vulkan/vulkan.hpp>
#include "debug.h"

namespace vulkan::task {
class PipelineFactory;
class CharacteristicRange;
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
class AddShaderStage;

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
  virtual AddShaderStage* get_add_shader_stage() { return nullptr; }
  virtual void register_AddShaderStage_with(task::PipelineFactory* pipeline_factory, task::CharacteristicRange const& characteristic_range_index) const { }
  virtual void set_set_index_hint_map(descriptor::SetIndexHintMap const* set_index_hint_map)
  {
    ASSERT(false);
    AI_NEVER_REACHED
  }
  virtual void pre_fill_state()
  {
    ASSERT(false);
    AI_NEVER_REACHED
  }
  virtual void preprocess_shaders_and_realize_descriptor_set_layouts(task::PipelineFactory* pipeline_factory)
  {
    ASSERT(false);
    AI_NEVER_REACHED
  }
  virtual void start_build_shaders()
  {
    ASSERT(false);
    AI_NEVER_REACHED
  }
  virtual bool build_shaders(task::CharacteristicRange* characteristic_range,
      task::PipelineFactory* pipeline_factory, AIStatefulTask::condition_type locked)
  {
    ASSERT(false);
    AI_NEVER_REACHED
  }
  //virtual void destroy_shader_module_handles() { }

  // Implemented by AddVertexShader.
  virtual void copy_shader_variables() { }
  virtual void update_vertex_input_descriptions() { }
  virtual void register_AddVertexShader_with(task::PipelineFactory* pipeline_factory, task::CharacteristicRange const& characteristic_range) const { }
  virtual void reset_vertex_shader_location_contex() { }

  // Implemented by AddFragmentShader.
  virtual void register_AddFragmentShader_with(task::PipelineFactory* pipeline_factory, task::CharacteristicRange const& characteristic_range) const { }

  // Implemented by CharacteristicRange.
  virtual shader_builder::ShaderResourceDeclaration* realize_shader_resource_declaration(std::string glsl_id_full, vk::DescriptorType descriptor_type, shader_builder::ShaderResourceBase const& shader_resource, descriptor::SetIndexHint set_index_hint) = 0;

  virtual task::PipelineFactory* get_owning_factory() const = 0;

  // Implemented by AddPushConstant.
  virtual void copy_push_constant_ranges(task::PipelineFactory* pipeline_factory) { }
  virtual void register_AddPushConstant_with(task::PipelineFactory* pipeline_factory, task::CharacteristicRange const& characteristic_range) const { }
  virtual void reset_push_constant_ranges() { }
};

} // namespace vulkan::pipeline
