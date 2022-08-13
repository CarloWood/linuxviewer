#pragma once

#include "ShaderVariableLayouts.h"      // standards::vertex_attributes
#include "math/glsl.h"
#include "utils/Vector.h"
#include <vulkan/vulkan.hpp>
#include <cstdint>
#include "debug.h"

namespace vulkan::pipeline {
class Pipeline;
} // namespace vulkan::pipeline

namespace vulkan::shaderbuilder {

// We need all members to be public because I want to use this as an aggregrate type.
struct BasicType
{
  static constexpr uint32_t standard_width_in_bits = std::bit_width(static_cast<uint32_t>(glsl::number_of_standards - 1));
  static constexpr uint32_t rows_width_in_bits = 3;      // 1, 2, 3 or 4.
  static constexpr uint32_t cols_width_in_bits = 3;      // 1, 2, 3 or 4.
  static_assert(standard_width_in_bits + rows_width_in_bits + cols_width_in_bits == 8, "That doesn't fit");
  static constexpr uint32_t scalar_type_width_in_bits = std::bit_width(static_cast<uint32_t>(glsl::number_of_scalar_types));    // 4 bits
  static constexpr uint32_t log2_alignment_width_in_bits = 8 - scalar_type_width_in_bits;       // Possible alignments are 4, 8, 16 and 32. So 2 bits would be sufficient (we use 3).
  static constexpr uint32_t size_width_in_bits = 8;
  static constexpr uint32_t array_stride_width_in_bits = 8;

  uint32_t       m_standard:standard_width_in_bits;             // See glsl::Standard.
  uint32_t           m_rows:rows_width_in_bits;                 // If rows is 1 then also cols is 1 and this is a Scalar.
  uint32_t           m_cols:cols_width_in_bits;                 // If cols is 1 then this is a Scalar or a Vector.
  uint32_t      m_scalar_type:scalar_type_width_in_bits;        // This type when this is a Scalar, otherwise the underlying scalar type.
  uint32_t m_log2_alignment:log2_alignment_width_in_bits{};     // The log2 of the alignment of this type when not used in an array.
  uint32_t           m_size:size_width_in_bits;                 // The (padded) size of this type when not used in an array.
  uint32_t   m_array_stride:array_stride_width_in_bits{};       // The (padded) size of this type when used in an array (always >= m_size).
                                                                // This size also fullfills the alignment: array_stride % alignment == 0.

 public:
  constexpr glsl::Standard standard() const { return static_cast<glsl::Standard>(m_standard); }
  constexpr int rows() const { return m_rows; }
  constexpr int cols() const { return m_cols; }
  constexpr glsl::Kind kind() const { return (m_rows == 1) ? glsl::Scalar : (m_cols == 1) ? glsl::Vector : glsl::Matrix; }
  constexpr glsl::ScalarIndex scalar_type() const { return static_cast<glsl::ScalarIndex>(m_scalar_type); }
  constexpr int alignment() const { ASSERT(m_standard != glsl::vertex_attributes); return 1 << m_log2_alignment; }
  constexpr int size() const { ASSERT(m_standard != glsl::vertex_attributes); return m_size; }
  constexpr int array_stride() const { return m_array_stride; }
  // This applies to Vertex Attributes (aka, see https://registry.khronos.org/OpenGL/specs/gl/GLSLangSpec.4.60.html 4.4.1).
  // An array consumes this per index.
  int consumed_locations() const { ASSERT(standard() == glsl::vertex_attributes); return ((scalar_type() == glsl::eDouble && rows() >= 3) ? 2 : 1) * cols(); }

#if 0
  constexpr operator uint32_t() const
  {
    return (((((m_array_stride <<
              size_width_in_bits | m_size) <<
    log2_alignment_width_in_bits | m_log2_alignment) <<
       scalar_type_width_in_bits | m_scalar_type) <<
              cols_width_in_bits | m_cols) <<
              rows_width_in_bits | m_rows) <<
          standard_width_in_bits | m_standard;
  }
#endif

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

//static_assert(sizeof(BasicType) == sizeof(uint32_t), "Size of BasicType is too large for encoding.");

class VertexShaderInputSetBase;
using BindingIndex = utils::VectorIndex<VertexShaderInputSetBase*>;

struct VertexAttributeLayout
{
  BasicType const m_base_type;                  // The underlying glsl base type of the variable.
  char const* const m_glsl_id_str;              // The glsl name of the variable (unhashed).
  uint32_t const m_offset;                      // The offset of the variable inside its C++ ENTRY struct.
  uint32_t const m_array_size;                  // Set to zero when this is not an array.

  std::string name() const;

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

struct VertexAttribute
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

  std::string declaration(pipeline::Pipeline* pipeline) const;

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

#ifdef CWDEBUG
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
