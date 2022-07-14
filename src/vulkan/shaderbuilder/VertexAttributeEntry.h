#pragma once

#include "ShaderVariableAttribute.h"
#include "utils/Vector.h"

namespace vulkan::shaderbuilder {

class VertexShaderInputSetBase;
using BindingIndex = utils::VectorIndex<VertexShaderInputSetBase*>;

struct VertexAttributeEntry : public ShaderVariableAttribute
{
  BindingIndex binding;

  VertexAttributeEntry(BindingIndex binding_, ShaderVariableAttribute const& shader_variable_attribute_) :
    ShaderVariableAttribute(shader_variable_attribute_), binding(binding_)
  {
    DoutEntering(dc::vulkan, "VertexAttributeEntry::VertexAttributeEntry(" << binding_ << ", " << shader_variable_attribute_ << ")");
    m_declaration = &VertexAttributeEntry::declaration;
  }

  static std::string declaration(ShaderVariableAttribute const* base, pipeline::Pipeline* pipeline);
};

} // namespace vulkan::shaderbuilder
