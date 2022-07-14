#pragma once

#include "ShaderVariableLayout.h"
#include "utils/Vector.h"

namespace vulkan::shaderbuilder {

class VertexShaderInputSetBase;
using BindingIndex = utils::VectorIndex<VertexShaderInputSetBase*>;

struct VertexAttribute : public ShaderVariableLayout
{
  BindingIndex binding;

  VertexAttribute(BindingIndex binding_, ShaderVariableLayout const& shader_variable_layout) :
    ShaderVariableLayout(shader_variable_layout), binding(binding_)
  {
    DoutEntering(dc::vulkan, "VertexAttribute::VertexAttribute(" << binding_ << ", " << shader_variable_layout << ")");
    m_declaration = &VertexAttribute::declaration;
  }

  static std::string declaration(ShaderVariableLayout const* base, pipeline::Pipeline* pipeline);
};

} // namespace vulkan::shaderbuilder
