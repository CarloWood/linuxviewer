#pragma once

#include "math/glsl.h"
#include <vulkan/vulkan.hpp>
#include "utils/Vector.h"
#include "utils/log2.h"
#include <functional>
#include <cstring>
#include <iosfwd>
#include <map>
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
  uint32_t      m_scalar_type:scalar_type_width_in_bits;            // This type when this is a Scalar, otherwise the underlying scalar type.
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
  constexpr int array_stride() const { ASSERT(m_standard != glsl::vertex_attributes); return m_array_stride; }
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

namespace standards {
using Standard = glsl::Standard;
using ScalarIndex = glsl::ScalarIndex;

using ScalarIndex::eFloat;
using ScalarIndex::eDouble;
using ScalarIndex::eBool;
using ScalarIndex::eInt;
using ScalarIndex::eUint;
using ScalarIndex::eInt8;
using ScalarIndex::eUint8;
using ScalarIndex::eInt16;
using ScalarIndex::eUint16;

template<Standard standard, ScalarIndex scalar_index, int rows, int cols, size_t alignment, size_t size, size_t array_stride>
struct BasicTypeLayout
{
};

template<typename TypeInfo>
struct Layout;

template<Standard standard, ScalarIndex scalar_index, int rows, int cols, size_t alignment_, size_t size_, size_t array_stride_>
struct Layout<BasicTypeLayout<standard, scalar_index, rows, cols, alignment_, size_, array_stride_>>
{
  static constexpr size_t alignment = alignment_;
  static constexpr size_t size = size_;
  static constexpr size_t array_stride = array_stride_;
};

#define SHADERBUILDER_STANDARD vertex_attributes
#include "base_types.h"
#undef SHADERBUILDER_STANDARD

#define SHADERBUILDER_STANDARD std140
#include "base_types.h"
#undef SHADERBUILDER_STANDARD

#define SHADERBUILDER_STANDARD std430
#include "base_types.h"
#undef SHADERBUILDER_STANDARD

#define SHADERBUILDER_STANDARD scalar
#include "base_types.h"
#undef SHADERBUILDER_STANDARD

} // namespace standards

struct TypeInfo
{
  std::string name;                             // glsl name
  int const number_of_attribute_indices;        // The number of sequential attribute indices that will be consumed.
  vk::Format const format;                      // The format to use for this type.

  TypeInfo(BasicType type);
};

// Must be an aggregate type.
struct ShaderVariableLayout
{
  BasicType const m_glsl_type;                  // The glsl type of the variable.
  char const* const m_glsl_id_str;              // The glsl name of the variable (unhashed).
  uint32_t const m_offset;                      // The offset of the variable inside its C++ ENTRY struct.
  std::string (*m_declaration)(ShaderVariableLayout const*, pipeline::Pipeline*);    // Pseudo virtual function that generates the declaration.

  std::string name() const;
  std::string declaration(pipeline::Pipeline* pipeline) const
  {
    // Set m_declaration when constructing the derived class. It's like a virtual function.
    ASSERT(m_declaration);
    return m_declaration(this, pipeline);
  }

  // ShaderVariableLayout are put in a boost::ptr_set. Provide a sorting function.
  bool operator<(ShaderVariableLayout const& other) const
  {
    return strcmp(m_glsl_id_str, other.m_glsl_id_str) < 0;
  }

  bool valid_alignment_and_size() const
  {
    auto standard = m_glsl_type.standard();
    return standard == glsl::std140 || standard == glsl::std430 || standard == glsl::scalar;
  }

  uint32_t alignment() const
  {
    return glsl::alignment(m_glsl_type.standard(), m_glsl_type.scalar_type(), m_glsl_type.rows(), m_glsl_type.cols());
  }

  uint32_t size() const
  {
    return glsl::size(m_glsl_type.standard(), m_glsl_type.scalar_type(), m_glsl_type.rows(), m_glsl_type.cols());
  }

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

// From https://registry.khronos.org/OpenGL/specs/gl/GLSLangSpec.4.60.html 4.4.5. Uniform and Shader Storage Block Layout Qualifiers:
//
// The std140 and std430 qualifiers override only the packed, shared, std140, and std430 qualifiers; other qualifiers are inherited.
// The std430 qualifier is supported only for shader storage blocks; a shader using the std430 qualifier on a uniform block will
// result in a compile-time error, unless it is also declared with push_constant.
//
// There is an extention that allows to use std430, but we don't use that yet.
struct uniform_std140 : vulkan::shaderbuilder::standards::std140::TypeEncodings
{
  using tag_type = uniform_std140;
};

// From the same paragraph:
//
// However, when push_constant is declared, the default layout of the buffer will be std430. There is no method to globally set this default.
struct push_constant_std430 : vulkan::shaderbuilder::standards::std430::TypeEncodings
{
  using tag_type = push_constant_std430;
};

// The following layouts might require an extension.
#if 1
struct uniform_std430 : vulkan::shaderbuilder::standards::std430::TypeEncodings
{
  using tag_type = uniform_std430;
};

struct uniform_scalar : vulkan::shaderbuilder::standards::scalar::TypeEncodings
{
  using tag_type = uniform_scalar;
};

struct push_constant_scalar : vulkan::shaderbuilder::standards::scalar::TypeEncodings
{
  using tag_type = push_constant_scalar;
};
#endif

} // glsl
