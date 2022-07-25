#pragma once

#include <vulkan/vulkan.hpp>
#include "utils/Vector.h"
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

// Bit encoding of Type:
//
// ttttssscccrrr
//
// row=0 -> [ e00,  e10 ]
// row=1 -> [ e01,  e11 ]
// row=2 -> [ e02,  e12 ]
// row=3 -> [ e03,  e13 ]  is stored in memory as { e00, e01, e02, e03, e10, e11, e12, e13 } (column-major).
//             ^     ^
//             |     |
//           col=0 col=1
//
// where tttt: the underlaying type.
//        sss: the size of the underlaying C++ type of one element.
//        ccc: the number of columns.
//        rrr: the number of rows.
//
// GLSL has the notation matCxR - with C cols and R rows.
// Eigen, and this library if I can help it, puts rows first.
//
// Vectors in glsl are neither row- or colum-vectors; or both. When multiplying a matrix with a vector
// (matNxM * vecN --> vecM) then the vector is treated like a column vector and the result is a column vector.
// But when multiplying a vector with a matrix (vecM * matNxM --> vecN) then the vector is treated like a
// row vector and the result is a row vector. When multiplying a vector with a vector (vecN * vecN --> vecN)
// they are are multiplied component-wise ({ x1, y1, z1 } * { x2, y2, z2 } = { x1 * x2, y1 *y2, z1 * z2 }).
//
static consteval int encode(int rows, int cols, int typesize, int typemask)
{
  return rows + (cols << 3) + (typesize << 6) + (typemask << 9);
}

static constexpr int float_mask   = 0;
static constexpr int double_mask  = 1;
static constexpr int bool_mask    = 2;
static constexpr int int32_mask   = 3;
static constexpr int uint32_mask  = 4;
static constexpr int snorm8_mask  = 5;
static constexpr int unorm8_mask  = 6;
static constexpr int snorm16_mask  = 7;
static constexpr int unorm16_mask  = 8;

// The vectors are encoded as column-vectors, because that's what the corresponding Eigen types are that we use.
enum class Type
{
  Float = encode(1, 1, sizeof(float), float_mask),
  vec2  = encode(2, 1, sizeof(float), float_mask),
  vec3  = encode(3, 1, sizeof(float), float_mask),
  vec4  = encode(4, 1, sizeof(float), float_mask),
  mat2  = encode(2, 2, sizeof(float), float_mask),
  mat3x2= encode(2, 3, sizeof(float), float_mask),
  mat4x2= encode(2, 4, sizeof(float), float_mask),
  mat2x3= encode(3, 2, sizeof(float), float_mask),
  mat3  = encode(3, 3, sizeof(float), float_mask),
  mat4x3= encode(3, 4, sizeof(float), float_mask),
  mat2x4= encode(4, 2, sizeof(float), float_mask),
  mat3x4= encode(4, 3, sizeof(float), float_mask),
  mat4  = encode(4, 4, sizeof(float), float_mask),

  Double  = encode(1, 1, sizeof(double), double_mask),
  dvec2   = encode(2, 1, sizeof(double), double_mask),
  dvec3   = encode(3, 1, sizeof(double), double_mask),
  dvec4   = encode(4, 1, sizeof(double), double_mask),
  dmat2   = encode(2, 2, sizeof(double), double_mask),
  dmat3x2 = encode(2, 3, sizeof(double), double_mask),
  dmat4x2 = encode(2, 4, sizeof(double), double_mask),
  dmat2x3 = encode(3, 2, sizeof(double), double_mask),
  dmat3   = encode(3, 3, sizeof(double), double_mask),
  dmat4x3 = encode(3, 4, sizeof(double), double_mask),
  dmat2x4 = encode(4, 2, sizeof(double), double_mask),
  dmat3x4 = encode(4, 3, sizeof(double), double_mask),
  dmat4   = encode(4, 4, sizeof(double), double_mask),

  Bool  = encode(1, 1, sizeof(bool), bool_mask),
  bvec2 = encode(2, 1, sizeof(bool), bool_mask),
  bvec3 = encode(3, 1, sizeof(bool), bool_mask),
  bvec4 = encode(4, 1, sizeof(bool), bool_mask),

  Int   = encode(1, 1, sizeof(int32_t), int32_mask),
  ivec2 = encode(2, 1, sizeof(int32_t), int32_mask),
  ivec3 = encode(3, 1, sizeof(int32_t), int32_mask),
  ivec4 = encode(4, 1, sizeof(int32_t), int32_mask),

  Uint  = encode(1, 1, sizeof(uint32_t), uint32_mask),
  uvec2 = encode(2, 1, sizeof(uint32_t), uint32_mask),
  uvec3 = encode(3, 1, sizeof(uint32_t), uint32_mask),
  uvec4 = encode(4, 1, sizeof(uint32_t), uint32_mask),

  Int8  =  encode(1, 1, sizeof(int8_t), snorm8_mask),
  i8vec2 = encode(2, 1, sizeof(int8_t), snorm8_mask),
  i8vec3 = encode(3, 1, sizeof(int8_t), snorm8_mask),
  i8vec4 = encode(4, 1, sizeof(int8_t), snorm8_mask),

  Uint8  = encode(1, 1, sizeof(uint8_t), unorm8_mask),
  u8vec2 = encode(2, 1, sizeof(uint8_t), unorm8_mask),
  u8vec3 = encode(3, 1, sizeof(uint8_t), unorm8_mask),
  u8vec4 = encode(4, 1, sizeof(uint8_t), unorm8_mask),

  Int16  =  encode(1, 1, sizeof(int16_t), snorm16_mask),
  i16vec2 = encode(2, 1, sizeof(int16_t), snorm16_mask),
  i16vec3 = encode(3, 1, sizeof(int16_t), snorm16_mask),
  i16vec4 = encode(4, 1, sizeof(int16_t), snorm16_mask),

  Uint16  = encode(1, 1, sizeof(uint16_t), unorm16_mask),
  u16vec2 = encode(2, 1, sizeof(uint16_t), unorm16_mask),
  u16vec3 = encode(3, 1, sizeof(uint16_t), unorm16_mask),
  u16vec4 = encode(4, 1, sizeof(uint16_t), unorm16_mask)
};

inline int decode_rows(Type type)
{
  return static_cast<int>(type) & 0x7;
}

inline int decode_cols(Type type)
{
  return (static_cast<int>(type) >> 3) & 0x7;
}

inline int decode_typesize(Type type)
{
  return (static_cast<int>(type) >> 6) & 0x7;
}

inline int decode_typemask(Type type)
{
  return static_cast<int>(type) >> 9;
}

struct TypeInfo
{
  char const* name;                             // glsl name
  size_t const size;                            // The size of the type in bytes (in C++).
  int const number_of_attribute_indices;        // The number of sequential attribute indices that will be consumed.
  vk::Format const format;                      // The format to use for this type.

  TypeInfo(Type type);
};

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
