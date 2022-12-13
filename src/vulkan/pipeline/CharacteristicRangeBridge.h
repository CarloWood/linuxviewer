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
  // Implemented by AddVertexShader.
  virtual utils::Vector<shader_builder::VertexShaderInputSetBase*> const& vertex_shader_input_sets() const { ASSERT(false); AI_NEVER_REACHED }

  // Implemented by CharacteristicRange.
  virtual shader_builder::ShaderResourceDeclaration* realize_shader_resource_declaration(std::string glsl_id_full, vk::DescriptorType descriptor_type, shader_builder::ShaderResourceBase const& shader_resource, descriptor::SetIndexHint set_index_hint) = 0;

  virtual task::PipelineFactory* get_owning_factory() const = 0;
};

} // namespace vulkan::pipeline
