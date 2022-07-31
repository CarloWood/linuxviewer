// This header is included three times from  ShaderVariableLayout.h, once for each layout standard.
// It is included while inside namespace vulkan::shaderbuilder::<layout_standard> - where <layout_standard>
// is one of: std140, std430 or scalar.
//
// The variable glsl_standard must be defined as glsl::std140, glsl::std430 or glsl::scalar respectively.

using namespace standards;

constexpr Type encode(int rows, int cols, glsl::ScalarIndex scalar_type)
{
  return Type{
    .m_standard = glsl_standard,
    .m_rows = static_cast<uint32_t>(rows),
    .m_cols = static_cast<uint32_t>(cols),
    .m_scalar_type = static_cast<uint32_t>(scalar_type),
    .m_log2_alignment = static_cast<uint32_t>(utils::log2(alignment(glsl_standard, scalar_type, rows, cols))),
    .m_size = size(glsl_standard, scalar_type, rows, cols),
    .m_array_stride = array_stride(glsl_standard, scalar_type, rows, cols)
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
};

struct Tag
{
  using tag_type = Tag;
  using Type = Type;
};
