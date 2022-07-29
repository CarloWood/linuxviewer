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
struct Type
{
  static constexpr uint32_t standard_width_in_bits = std::bit_width(static_cast<uint32_t>(glsl::number_of_standards - 1));
  static constexpr uint32_t rows_width_in_bits = 3;      // 1, 2, 3 or 4.
  static constexpr uint32_t cols_width_in_bits = 3;      // 1, 2, 3 or 4.
  static_assert(standard_width_in_bits + rows_width_in_bits + cols_width_in_bits == 8, "That doesn't fit");
  static constexpr uint32_t base_type_width_in_bits = std::bit_width(static_cast<uint32_t>(glsl::number_of_base_types));        // 4 bits
  static constexpr uint32_t log2_alignment_width_in_bits = 8 - base_type_width_in_bits;       // Possible alignments are 4, 8, 16 and 32. So 2 bits would be sufficient (we use 3).
  static constexpr uint32_t size_width_in_bits = 8;
  static constexpr uint32_t array_stride_width_in_bits = 8;

  uint32_t       m_standard:standard_width_in_bits;             // See glsl::Standard.
  uint32_t           m_rows:rows_width_in_bits;                 // If rows is 1 then also cols is 1 and this is a Scalar.
  uint32_t           m_cols:cols_width_in_bits;                 // If cols is 1 then this is a Scalar or a Vector.
  uint32_t      m_base_type:base_type_width_in_bits;            // This type when this is a Scalar, otherwise the underlaying scalar type.
  uint32_t m_log2_alignment:log2_alignment_width_in_bits{};     // The log2 of the alignment of this type when not used in an array.
  uint32_t           m_size:size_width_in_bits;                 // The (padded) size of this type when not used in an array.
  uint32_t   m_array_stride:array_stride_width_in_bits{};

 public:
  glsl::Standard standard() const { return static_cast<glsl::Standard>(m_standard); }
  int rows() const { return m_rows; }
  int cols() const { return m_cols; }
  glsl::Kind kind() const { return (m_rows == 1) ? glsl::Scalar : (m_cols == 1) ? glsl::Vector : glsl::Matrix; }
  glsl::TypeIndex base_type() const { return static_cast<glsl::TypeIndex>(m_base_type); }
  int alignment() const { return 1 << m_log2_alignment; }
  int size() const { return m_size; }
  int array_stride() const { return m_array_stride; }
  // This applies to Vertex Attributes (aka, see https://registry.khronos.org/OpenGL/specs/gl/GLSLangSpec.4.60.html 4.4.1).
  // An array consumes this per index.
  int consumed_locations() const { return ((m_base_type == glsl::eDouble && m_rows >= 3) ? 2 : 1) * m_cols; }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

static_assert(sizeof(Type) == sizeof(uint32_t), "Size of Type is too large for encoding.");

namespace standards {
  using TI = glsl::TypeIndex;
} // namespace standards

namespace vertex_attributes {
using namespace standards;

constexpr Type encode(int rows, int cols, int base_type)
{
  return Type{
    .m_standard = glsl::vertex_attributes,
    .m_rows = static_cast<uint32_t>(rows),
    .m_cols = static_cast<uint32_t>(cols),
    .m_base_type = static_cast<uint32_t>(base_type),
    .m_size = (base_type == TI::eDouble) ? 8U : 4U
  };
}

// All the vectors are encoded as column-vectors, because you have to pick something.
struct Type
{
  static constexpr auto Float   = encode(1, 1, TI::eFloat);
  static constexpr auto vec2    = encode(2, 1, TI::eFloat);
  static constexpr auto vec3    = encode(3, 1, TI::eFloat);
  static constexpr auto vec4    = encode(4, 1, TI::eFloat);
  static constexpr auto mat2    = encode(2, 2, TI::eFloat);
  static constexpr auto mat3x2  = encode(2, 3, TI::eFloat);
  static constexpr auto mat4x2  = encode(2, 4, TI::eFloat);
  static constexpr auto mat2x3  = encode(3, 2, TI::eFloat);
  static constexpr auto mat3    = encode(3, 3, TI::eFloat);
  static constexpr auto mat4x3  = encode(3, 4, TI::eFloat);
  static constexpr auto mat2x4  = encode(4, 2, TI::eFloat);
  static constexpr auto mat3x4  = encode(4, 3, TI::eFloat);
  static constexpr auto mat4    = encode(4, 4, TI::eFloat);

  static constexpr auto Double  = encode(1, 1, TI::eDouble);
  static constexpr auto dvec2   = encode(2, 1, TI::eDouble);
  static constexpr auto dvec3   = encode(3, 1, TI::eDouble);
  static constexpr auto dvec4   = encode(4, 1, TI::eDouble);
  static constexpr auto dmat2   = encode(2, 2, TI::eDouble);
  static constexpr auto dmat3x2 = encode(2, 3, TI::eDouble);
  static constexpr auto dmat4x2 = encode(2, 4, TI::eDouble);
  static constexpr auto dmat2x3 = encode(3, 2, TI::eDouble);
  static constexpr auto dmat3   = encode(3, 3, TI::eDouble);
  static constexpr auto dmat4x3 = encode(3, 4, TI::eDouble);
  static constexpr auto dmat2x4 = encode(4, 2, TI::eDouble);
  static constexpr auto dmat3x4 = encode(4, 3, TI::eDouble);
  static constexpr auto dmat4   = encode(4, 4, TI::eDouble);

  static constexpr auto Bool    = encode(1, 1, TI::eBool);
  static constexpr auto bvec2   = encode(2, 1, TI::eBool);
  static constexpr auto bvec3   = encode(3, 1, TI::eBool);
  static constexpr auto bvec4   = encode(4, 1, TI::eBool);

  static constexpr auto Int     = encode(1, 1, TI::eInt);
  static constexpr auto ivec2   = encode(2, 1, TI::eInt);
  static constexpr auto ivec3   = encode(3, 1, TI::eInt);
  static constexpr auto ivec4   = encode(4, 1, TI::eInt);

  static constexpr auto Uint    = encode(1, 1, TI::eUint);
  static constexpr auto uvec2   = encode(2, 1, TI::eUint);
  static constexpr auto uvec3   = encode(3, 1, TI::eUint);
  static constexpr auto uvec4   = encode(4, 1, TI::eUint);

  static constexpr auto Int8    = encode(1, 1, TI::eInt8);
  static constexpr auto i8vec2  = encode(2, 1, TI::eInt8);
  static constexpr auto i8vec3  = encode(3, 1, TI::eInt8);
  static constexpr auto i8vec4  = encode(4, 1, TI::eInt8);

  static constexpr auto Uint8   = encode(1, 1, TI::eUint8);
  static constexpr auto u8vec2  = encode(2, 1, TI::eUint8);
  static constexpr auto u8vec3  = encode(3, 1, TI::eUint8);
  static constexpr auto u8vec4  = encode(4, 1, TI::eUint8);

  static constexpr auto Int16   = encode(1, 1, TI::eInt16);
  static constexpr auto i16vec2 = encode(2, 1, TI::eInt16);
  static constexpr auto i16vec3 = encode(3, 1, TI::eInt16);
  static constexpr auto i16vec4 = encode(4, 1, TI::eInt16);

  static constexpr auto Uint16  = encode(1, 1, TI::eUint16);
  static constexpr auto u16vec2 = encode(2, 1, TI::eUint16);
  static constexpr auto u16vec3 = encode(3, 1, TI::eUint16);
  static constexpr auto u16vec4 = encode(4, 1, TI::eUint16);
};

struct Tag
{
  using Type = Type;
};

} // namespace vertex_attributes

namespace std140 {
using namespace standards;

constexpr Type encode(int rows, int cols, int base_type)
{
  return Type{
    .m_standard = glsl::std140,
    .m_rows = static_cast<uint32_t>(rows),
    .m_cols = static_cast<uint32_t>(cols),
    .m_base_type = static_cast<uint32_t>(base_type),
    .m_size = (base_type == TI::eDouble) ? 8U : 4U      // FIXME?
  };
}

// All the vectors are encoded as column-vectors, because you have to pick something.
struct Type
{
  static constexpr auto Float   = encode(1, 1, TI::eFloat);
  static constexpr auto vec2    = encode(2, 1, TI::eFloat);
  static constexpr auto vec3    = encode(3, 1, TI::eFloat);
  static constexpr auto vec4    = encode(4, 1, TI::eFloat);
  static constexpr auto mat2    = encode(2, 2, TI::eFloat);
  static constexpr auto mat3x2  = encode(2, 3, TI::eFloat);
  static constexpr auto mat4x2  = encode(2, 4, TI::eFloat);
  static constexpr auto mat2x3  = encode(3, 2, TI::eFloat);
  static constexpr auto mat3    = encode(3, 3, TI::eFloat);
  static constexpr auto mat4x3  = encode(3, 4, TI::eFloat);
  static constexpr auto mat2x4  = encode(4, 2, TI::eFloat);
  static constexpr auto mat3x4  = encode(4, 3, TI::eFloat);
  static constexpr auto mat4    = encode(4, 4, TI::eFloat);

  static constexpr auto Double  = encode(1, 1, TI::eDouble);
  static constexpr auto dvec2   = encode(2, 1, TI::eDouble);
  static constexpr auto dvec3   = encode(3, 1, TI::eDouble);
  static constexpr auto dvec4   = encode(4, 1, TI::eDouble);
  static constexpr auto dmat2   = encode(2, 2, TI::eDouble);
  static constexpr auto dmat3x2 = encode(2, 3, TI::eDouble);
  static constexpr auto dmat4x2 = encode(2, 4, TI::eDouble);
  static constexpr auto dmat2x3 = encode(3, 2, TI::eDouble);
  static constexpr auto dmat3   = encode(3, 3, TI::eDouble);
  static constexpr auto dmat4x3 = encode(3, 4, TI::eDouble);
  static constexpr auto dmat2x4 = encode(4, 2, TI::eDouble);
  static constexpr auto dmat3x4 = encode(4, 3, TI::eDouble);
  static constexpr auto dmat4   = encode(4, 4, TI::eDouble);

  static constexpr auto Bool    = encode(1, 1, TI::eBool);
  static constexpr auto bvec2   = encode(2, 1, TI::eBool);
  static constexpr auto bvec3   = encode(3, 1, TI::eBool);
  static constexpr auto bvec4   = encode(4, 1, TI::eBool);
};

struct Tag
{
  using tag_type = Tag;
  using Type = Type;
};

} // namespace std140

namespace std430 {
using namespace standards;

constexpr Type encode(int rows, int cols, int base_type)
{
  return Type{
    .m_standard = glsl::std430,
    .m_rows = static_cast<uint32_t>(rows),
    .m_cols = static_cast<uint32_t>(cols),
    .m_base_type = static_cast<uint32_t>(base_type),
    .m_size = (base_type == TI::eDouble) ? 8U : 4U      // FIXME?
  };
}

// All the vectors are encoded as column-vectors, because you have to pick something.
struct Type
{
  static constexpr auto Float   = encode(1, 1, TI::eFloat);
  static constexpr auto vec2    = encode(2, 1, TI::eFloat);
  static constexpr auto vec3    = encode(3, 1, TI::eFloat);
  static constexpr auto vec4    = encode(4, 1, TI::eFloat);
  static constexpr auto mat2    = encode(2, 2, TI::eFloat);
  static constexpr auto mat3x2  = encode(2, 3, TI::eFloat);
  static constexpr auto mat4x2  = encode(2, 4, TI::eFloat);
  static constexpr auto mat2x3  = encode(3, 2, TI::eFloat);
  static constexpr auto mat3    = encode(3, 3, TI::eFloat);
  static constexpr auto mat4x3  = encode(3, 4, TI::eFloat);
  static constexpr auto mat2x4  = encode(4, 2, TI::eFloat);
  static constexpr auto mat3x4  = encode(4, 3, TI::eFloat);
  static constexpr auto mat4    = encode(4, 4, TI::eFloat);

  static constexpr auto Double  = encode(1, 1, TI::eDouble);
  static constexpr auto dvec2   = encode(2, 1, TI::eDouble);
  static constexpr auto dvec3   = encode(3, 1, TI::eDouble);
  static constexpr auto dvec4   = encode(4, 1, TI::eDouble);
  static constexpr auto dmat2   = encode(2, 2, TI::eDouble);
  static constexpr auto dmat3x2 = encode(2, 3, TI::eDouble);
  static constexpr auto dmat4x2 = encode(2, 4, TI::eDouble);
  static constexpr auto dmat2x3 = encode(3, 2, TI::eDouble);
  static constexpr auto dmat3   = encode(3, 3, TI::eDouble);
  static constexpr auto dmat4x3 = encode(3, 4, TI::eDouble);
  static constexpr auto dmat2x4 = encode(4, 2, TI::eDouble);
  static constexpr auto dmat3x4 = encode(4, 3, TI::eDouble);
  static constexpr auto dmat4   = encode(4, 4, TI::eDouble);

  static constexpr auto Bool    = encode(1, 1, TI::eBool);
  static constexpr auto bvec2   = encode(2, 1, TI::eBool);
  static constexpr auto bvec3   = encode(3, 1, TI::eBool);
  static constexpr auto bvec4   = encode(4, 1, TI::eBool);
};

struct Tag
{
  using tag_type = Tag;
  using Type = Type;
};

} // namespace std430

struct TypeInfo
{
  std::string name;                             // glsl name
  size_t const size;                            // The size of the type in bytes.
  int const number_of_attribute_indices;        // The number of sequential attribute indices that will be consumed.
  vk::Format const format;                      // The format to use for this type.

  TypeInfo(Type type);
};

// Must be an aggregate type.
struct ShaderVariableLayout
{
  Type const m_glsl_type;                       // The glsl type of the variable.
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

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::shaderbuilder

// Not sure where to put the 'standards' structs... lets put them in glsl for now.
namespace glsl {

struct per_vertex_data : vulkan::shaderbuilder::vertex_attributes::Tag
{
  using tag_type = per_vertex_data;
  static constexpr vk::VertexInputRate input_rate = vk::VertexInputRate::eVertex;       // This is per vertex data.
};

struct per_instance_data : vulkan::shaderbuilder::vertex_attributes::Tag
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
struct uniform_std140 : vulkan::shaderbuilder::std140::Tag
{
};

// From the same paragraph:
//
// However, when push_constant is declared, the default layout of the buffer will be std430. There is no method to globally set this default.
struct push_constant_std430 : vulkan::shaderbuilder::std430::Tag
{
};

} // glsl
