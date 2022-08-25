#pragma once

#include "ShaderVariableLayouts.h"      // standards::vertex_attributes
#include "ShaderVariable.h"
#include "BasicType.h"
#include "utils/Vector.h"
#include <vulkan/vulkan.hpp>
#include <cstdint>
#include "debug.h"

namespace vulkan::pipeline {
class ShaderInputData;
} // namespace vulkan::pipeline

namespace vulkan::shaderbuilder {

class VertexShaderInputSetBase;
using BindingIndex = utils::VectorIndex<VertexShaderInputSetBase*>;

struct VertexAttributeLayout
{
  BasicType const m_base_type;                  // The underlying glsl base type of the variable.
  char const* const m_glsl_id_str;              // The glsl name of the variable (unhashed).
  uint32_t const m_offset;                      // The offset of the variable inside its C++ ENTRY struct.
  uint32_t const m_array_size;                  // Set to zero when this is not an array.

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

struct VertexAttribute final : public ShaderVariable
{
 private:
  VertexAttributeLayout const* m_layout;
  BindingIndex m_binding;

 public:
  VertexAttributeLayout const& layout() const
  {
    return *m_layout;
  }

  BindingIndex binding() const
  {
    return m_binding;
  }

  VertexAttribute(VertexAttributeLayout const* layout, BindingIndex binding) : m_layout(layout), m_binding(binding)
  {
    DoutEntering(dc::vulkan, "VertexAttribute::VertexAttribute(&" << *layout << ", " << binding << ")");
  }

  // VertexAttribute are put in a std::set. Provide a sorting function.
  bool operator<(VertexAttribute const& other) const
  {
    return strcmp(m_layout->m_glsl_id_str, other.m_layout->m_glsl_id_str) < 0;
  }

#if 0
  bool valid_alignment_and_size() const
  {
    auto standard = m_base_type.standard();
    return standard == glsl::std140 || standard == glsl::std430 || standard == glsl::scalar;
  }

  uint32_t alignment() const
  {
    return glsl::alignment(m_base_type.standard(), m_base_type.scalar_type(), m_base_type.rows(), m_base_type.cols());
  }

  uint32_t size() const
  {
    return glsl::size(m_base_type.standard(), m_base_type.scalar_type(), m_base_type.rows(), m_base_type.cols());
  }
#endif

 public:
  char const* glsl_id_str() const override { return m_layout->m_glsl_id_str; }
  std::string name() const override;

 private:
  // Implement base class interface.
  DeclarationContext const& is_used_in(vk::ShaderStageFlagBits shader_stage, pipeline::ShaderInputData* shader_input_data) const override;

#ifdef CWDEBUG
 public:
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::shaderbuilder

// Not sure where to put the 'standards' structs... lets put them in glsl for now.
namespace glsl {

struct per_vertex_data : vulkan::shaderbuilder::standards::vertex_attributes::TypeEncodings
{
  using tag_type = per_vertex_data;
  static constexpr vk::VertexInputRate input_rate = vk::VertexInputRate::eVertex;       // This is per vertex data.
};

struct per_instance_data : vulkan::shaderbuilder::standards::vertex_attributes::TypeEncodings
{
  using tag_type = per_instance_data;
  static constexpr vk::VertexInputRate input_rate = vk::VertexInputRate::eInstance;     // This is per instance data.
};

} // glsl
