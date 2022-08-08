#pragma once

#include "ShaderVertexInputAttributeLayout.h"
#include "utils/Vector.h"

namespace vulkan::shaderbuilder {

class VertexShaderInputSetBase;
using BindingIndex = utils::VectorIndex<VertexShaderInputSetBase*>;

struct VertexAttribute : public ShaderVertexInputAttributeLayout
{
  BindingIndex binding;

  VertexAttribute(BindingIndex binding_, ShaderVertexInputAttributeLayout const& shader_variable_layout) :
    ShaderVertexInputAttributeLayout(shader_variable_layout), binding(binding_)
  {
    DoutEntering(dc::vulkan, "VertexAttribute::VertexAttribute(" << binding_ << ", " << shader_variable_layout << ")");
    m_declaration = &VertexAttribute::declaration;
  }

  static std::string declaration(ShaderVertexInputAttributeLayout const* base, pipeline::Pipeline* pipeline);
};

} // namespace vulkan::shaderbuilder
