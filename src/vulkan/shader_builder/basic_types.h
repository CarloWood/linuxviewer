// This header is included four times from  ShaderVariableLayout.h, once for each layout standard.
// It is included while SHADERBUILDER_STANDARD is defined to out of the standards, one of: vertex_attributes,
// std140, std430 or scalar.

#define vertex_attributes 1029384756
#if SHADERBUILDER_STANDARD == vertex_attributes
#define VULKAN_SHADERBUILDER_in_VERTEX_ATTRIBUTES 1
#else
#define VULKAN_SHADERBUILDER_in_VERTEX_ATTRIBUTES 0
#endif
#undef vertex_attributes

namespace SHADERBUILDER_STANDARD {

// The variable glsl_standard must be set to one of the glsl::Standard enum values.
static constexpr Standard glsl_standard = glsl::SHADERBUILDER_STANDARD;

template<int rows, int cols, glsl::ScalarIndex scalar_index>
using encode = BasicTypeLayout<glsl_standard, scalar_index, rows, cols, alignment(glsl_standard, scalar_index, rows, cols), size(glsl_standard, scalar_index, rows, cols), array_stride(glsl_standard, scalar_index, rows, cols)>;

struct TypeEncodings
{
  // So we can access this from glsl 'standards' structs.
  static constexpr Standard glsl_standard = glsl::SHADERBUILDER_STANDARD;

  // All the vectors are encoded as column-vectors, because you have to pick something.
  using Float   = encode<1, 1, eFloat>;
  using vec2    = encode<2, 1, eFloat>;
  using vec3    = encode<3, 1, eFloat>;
  using vec4    = encode<4, 1, eFloat>;
  using mat2    = encode<2, 2, eFloat>;
  using mat3x2  = encode<2, 3, eFloat>;
  using mat4x2  = encode<2, 4, eFloat>;
  using mat2x3  = encode<3, 2, eFloat>;
  using mat3    = encode<3, 3, eFloat>;
  using mat4x3  = encode<3, 4, eFloat>;
  using mat2x4  = encode<4, 2, eFloat>;
  using mat3x4  = encode<4, 3, eFloat>;
  using mat4    = encode<4, 4, eFloat>;

  using Double  = encode<1, 1, eDouble>;
  using dvec2   = encode<2, 1, eDouble>;
  using dvec3   = encode<3, 1, eDouble>;
  using dvec4   = encode<4, 1, eDouble>;
  using dmat2   = encode<2, 2, eDouble>;
  using dmat3x2 = encode<2, 3, eDouble>;
  using dmat4x2 = encode<2, 4, eDouble>;
  using dmat2x3 = encode<3, 2, eDouble>;
  using dmat3   = encode<3, 3, eDouble>;
  using dmat4x3 = encode<3, 4, eDouble>;
  using dmat2x4 = encode<4, 2, eDouble>;
  using dmat3x4 = encode<4, 3, eDouble>;
  using dmat4   = encode<4, 4, eDouble>;

  using Bool    = encode<1, 1, eBool>;
  using bvec2   = encode<2, 1, eBool>;
  using bvec3   = encode<3, 1, eBool>;
  using bvec4   = encode<4, 1, eBool>;

  using Int     = encode<1, 1, eInt>;
  using ivec2   = encode<2, 1, eInt>;
  using ivec3   = encode<3, 1, eInt>;
  using ivec4   = encode<4, 1, eInt>;

  using Uint    = encode<1, 1, eUint>;
  using uvec2   = encode<2, 1, eUint>;
  using uvec3   = encode<3, 1, eUint>;
  using uvec4   = encode<4, 1, eUint>;

#if VULKAN_SHADERBUILDER_in_VERTEX_ATTRIBUTES
  using Int8    = encode<1, 1, eInt8>;
  using i8vec2  = encode<2, 1, eInt8>;
  using i8vec3  = encode<3, 1, eInt8>;
  using i8vec4  = encode<4, 1, eInt8>;

  using Uint8   = encode<1, 1, eUint8>;
  using u8vec2  = encode<2, 1, eUint8>;
  using u8vec3  = encode<3, 1, eUint8>;
  using u8vec4  = encode<4, 1, eUint8>;

  using Int16   = encode<1, 1, eInt16>;
  using i16vec2 = encode<2, 1, eInt16>;
  using i16vec3 = encode<3, 1, eInt16>;
  using i16vec4 = encode<4, 1, eInt16>;

  using Uint16  = encode<1, 1, eUint16>;
  using u16vec2 = encode<2, 1, eUint16>;
  using u16vec3 = encode<3, 1, eUint16>;
  using u16vec4 = encode<4, 1, eUint16>;
#endif
};

} // namespace SHADERBUILDER_STANDARD

#undef VULKAN_SHADERBUILDER_in_VERTEX_ATTRIBUTES
