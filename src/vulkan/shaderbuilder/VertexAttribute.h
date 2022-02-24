#pragma once

#include <cstring>
#include <iosfwd>
#include <map>

namespace vulkan::shaderbuilder {

enum class Type
{
  Bool,
  Int,
  Uint,
  Float,
  Double,
  bvec2,
  bvec3,
  bvec4,
  ivec2,
  ivec3,
  ivec4,
  uvec2,
  uvec3,
  uvec4,
  vec2,
  vec3,
  vec4,
  dvec2,
  dvec3,
  dvec4,
  mat2,
  mat2x3,
  mat2x4,
  mat3,
  mat3x4,
  mat4,
  dmat2,
  dmat2x3,
  dmat2x4,
  dmat3,
  dmat3x4,
  dmat4
};

struct TypeInfo
{
  char const* name;                     // glsl name
  size_t size;                          // The size of the type in bytes (in C++).
  int number_of_attribute_indices;      // The number of sequential attribute indices that will be consumed.

  TypeInfo(Type type);
};

struct VertexAttribute;

struct LocationContext
{
  int next_location = 0;
  std::map<VertexAttribute const*, int> locations;

  void update_location(VertexAttribute const* vertex_attribute);
};

struct VertexAttribute
{
  Type const m_type;                    // The glsl type of the variable.
  char const* const m_glsl_id_str;      // The glsl name of the variable.
  size_t const m_offset;                // The offset of the attribute inside its C++ InputObject struct.

  bool operator<(VertexAttribute const& other) const
  {
    return strcmp(m_glsl_id_str, other.m_glsl_id_str) < 0 ;
  }

  std::string name() const;
  std::string declaration(LocationContext& context) const;

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::shaderbuilder
