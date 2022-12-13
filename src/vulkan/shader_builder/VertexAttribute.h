#pragma once

#include "ShaderVariableLayouts.h"      // standards::vertex_attributes
#include "ShaderVariable.h"
#include "BasicType.h"
#include "utils/Vector.h"
#include <vulkan/vulkan.hpp>
#include <cstdint>
#include "debug.h"

namespace vulkan::shader_builder {

class VertexShaderInputSetBase;
using BindingIndex = utils::VectorIndex<VertexShaderInputSetBase*>;

struct VertexAttribute final : public ShaderVariable
{
 private:
  BasicType const m_basic_type;                 // The glsl basic type of the variable, or underlying basic type in case of an array.
  uint32_t const m_offset;                      // The offset of the variable inside its C++ ENTRY struct.
  uint32_t const m_array_size;                  // Set to zero when this is not an array.
  BindingIndex m_binding;

 public:
  // Accessors.
  BasicType basic_type() const { return m_basic_type; }
  uint32_t offset() const { return m_offset; }
  uint32_t array_size() const { return m_array_size; }
  BindingIndex binding() const { return m_binding; }

  VertexAttribute(BasicType basic_type, char const* glsl_id_full, uint32_t offset, uint32_t array_size, BindingIndex binding) :
    ShaderVariable(glsl_id_full), m_basic_type(basic_type), m_offset(offset), m_array_size(array_size), m_binding(binding)
  {
    DoutEntering(dc::vulkan, "VertexAttribute::VertexAttribute(" << basic_type << ", \"" << glsl_id_full << "\", " << ", " << offset << ", " << array_size << ", " << binding << ")");
  }

  // VertexAttribute are put in a std::map. Provide a sorting function.
  bool operator<(VertexAttribute const& other) const
  {
    return strcmp(m_glsl_id_full, other.m_glsl_id_full) < 0;
  }

#if 0
  bool valid_alignment_and_size() const
  {
    auto standard = m_basic_type.standard();
    return standard == glsl::std140 || standard == glsl::std430 || standard == glsl::scalar;
  }

  uint32_t alignment() const
  {
    return glsl::alignment(m_basic_type.standard(), m_basic_type.scalar_type(), m_basic_type.rows(), m_basic_type.cols());
  }

  uint32_t size() const
  {
    return glsl::size(m_basic_type.standard(), m_basic_type.scalar_type(), m_basic_type.rows(), m_basic_type.cols());
  }
#endif

 public:
  std::string name() const override;

 private:
  // Implement base class interface.
  DeclarationContext* is_used_in(vk::ShaderStageFlagBits shader_stage, pipeline::AddShaderVariableDeclaration* add_shader_variable_declaration) const override;

#ifdef CWDEBUG
 public:
  void print_on(std::ostream& os) const override;
#endif
};

} // namespace vulkan::shader_builder

// Not sure where to put the 'standards' structs... lets put them in glsl for now.
namespace glsl {

struct per_vertex_data : vulkan::shader_builder::standards::vertex_attributes::TypeEncodings
{
  using tag_type = per_vertex_data;
  static constexpr vk::VertexInputRate input_rate = vk::VertexInputRate::eVertex;       // This is per vertex data.
};

struct per_instance_data : vulkan::shader_builder::standards::vertex_attributes::TypeEncodings
{
  using tag_type = per_instance_data;
  static constexpr vk::VertexInputRate input_rate = vk::VertexInputRate::eInstance;     // This is per instance data.
};

} // glsl
