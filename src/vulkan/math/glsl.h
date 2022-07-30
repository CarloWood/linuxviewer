#pragma once

#include <Eigen/Core>

namespace glsl {

static constexpr int number_of_standards = 4;
enum Standard {
  vertex_attributes,
  std140,
  std430,
  scalar
};

// Basic types.

// Builtin-types indices.
static constexpr int number_of_glsl_types = 5;
static constexpr int number_of_scalar_types = 9;
enum ScalarIndex {
  eFloat  = 0,
  eDouble = 1,
  eBool   = 2,
  eInt    = 3,
  eUint   = 4,
  // The following types are only for vertex_attributes; they do not exist in glsl.
  eInt8   = 5,
  eUint8  = 6,
  eInt16  = 7,
  eUint16 = 8
};

std::string to_string(ScalarIndex scalar_type);    // Prints the glsl type, not the name of the enum (aka, prints all lower case and without the leading 'e').

enum Kind {
  Scalar,
  Vector,
  Matrix
};

#ifdef CWDEBUG
std::string to_string(Standard standard);
std::ostream& operator<<(std::ostream& os, ScalarIndex scalar_type);
#endif

// GLSL has the notation matCxR - with C cols and R rows.
// Eigen, and this library if I can help it, puts rows first.
// The type used for vectors are all column-vectors (with 1 column).
//
// The basic types of GLSL can be found here: https://registry.khronos.org/OpenGL/specs/gl/GLSLangSpec.4.60.pdf [4.1 Basic Types].
//
// Note: vectors in glsl are neither row- or colum-vectors; or both. When multiplying a matrix with a vector
// (matNxM * vecN --> vecM) then the vector is treated like a column vector and the result is a column vector.
// But when multiplying a vector with a matrix (vecM * matNxM --> vecN) then the vector is treated like a
// row vector and the result is a row vector. When multiplying a vector with a vector (vecN * vecN --> vecN)
// they are are multiplied component-wise ({ x1, y1, z1 } * { x2, y2, z2 } = { x1 * x2, y1 *y2, z1 * z2 }).

// Allowed types for glsl.
using Float  = float;
using vec2   = Eigen::Vector2f;
using vec3   = Eigen::Vector3f;
using vec4   = Eigen::Vector4f;
using mat2   = Eigen::Matrix2f;
using mat3x2 = Eigen::Matrix<float, 2, 3>;
using mat4x2 = Eigen::Matrix<float, 2, 4>;
using mat2x3 = Eigen::Matrix<float, 3, 2>;
using mat3   = Eigen::Matrix3f;
using mat4x3 = Eigen::Matrix<float, 3, 4>;
using mat2x4 = Eigen::Matrix<float, 4, 2>;
using mat3x4 = Eigen::Matrix<float, 4, 3>;
using mat4   = Eigen::Matrix4f;

using Double  = double;
using dvec2   = Eigen::Vector2d;
using dvec3   = Eigen::Vector3d;
using dvec4   = Eigen::Vector4d;
using dmat2   = Eigen::Matrix2d;
using dmat3x2 = Eigen::Matrix<double, 2, 3>;
using dmat4x2 = Eigen::Matrix<double, 2, 4>;
using dmat2x3 = Eigen::Matrix<double, 3, 2>;
using dmat3   = Eigen::Matrix3d;
using dmat4x3 = Eigen::Matrix<double, 3, 4>;
using dmat2x4 = Eigen::Matrix<double, 4, 2>;
using dmat3x4 = Eigen::Matrix<double, 4, 3>;
using dmat4   = Eigen::Matrix4d;

// There are no matrices for boolean or integers.

using Bool   = bool;
using bvec2  = Eigen::Matrix<bool, 2, 1>;
using bvec3  = Eigen::Matrix<bool, 3, 1>;
using bvec4  = Eigen::Matrix<bool, 4, 1>;

using Int    = int32_t;
using ivec2  = Eigen::Matrix<int32_t, 2, 1>;
using ivec3  = Eigen::Matrix<int32_t, 3, 1>;
using ivec4  = Eigen::Matrix<int32_t, 4, 1>;

using Uint   = uint32_t;
using uvec2  = Eigen::Matrix<uint32_t, 2, 1>;
using uvec3  = Eigen::Matrix<uint32_t, 3, 1>;
using uvec4  = Eigen::Matrix<uint32_t, 4, 1>;

using Int8   = int8_t;
using i8vec2 = Eigen::Matrix<int8_t, 2, 1>;
using i8vec3 = Eigen::Matrix<int8_t, 3, 1>;
using i8vec4 = Eigen::Matrix<int8_t, 4, 1>;

using Uint8  = uint8_t;
using u8vec2 = Eigen::Matrix<uint8_t, 2, 1>;
using u8vec3 = Eigen::Matrix<uint8_t, 3, 1>;
using u8vec4 = Eigen::Matrix<uint8_t, 4, 1>;

using Int16  = int16_t;
using i16vec2 = Eigen::Matrix<int16_t, 2, 1>;
using i16vec3 = Eigen::Matrix<int16_t, 3, 1>;
using i16vec4 = Eigen::Matrix<int16_t, 4, 1>;

using Uint16 = uint16_t;
using u16vec2 = Eigen::Matrix<uint16_t, 2, 1>;
using u16vec3 = Eigen::Matrix<uint16_t, 3, 1>;
using u16vec4 = Eigen::Matrix<uint16_t, 4, 1>;

// All of these types must be dense float arrays.

static_assert(
    sizeof(vec2)   ==     2 * sizeof(float) &&
    sizeof(vec3)   ==     3 * sizeof(float) &&
    sizeof(vec4)   ==     4 * sizeof(float) &&
    sizeof(mat2)   == 2 * 2 * sizeof(float) &&
    sizeof(mat3x2) == 3 * 2 * sizeof(float) &&
    sizeof(mat4x2) == 4 * 2 * sizeof(float) &&
    sizeof(mat2x3) == 2 * 3 * sizeof(float) &&
    sizeof(mat3)   == 3 * 3 * sizeof(float) &&
    sizeof(mat4x3) == 4 * 3 * sizeof(float) &&
    sizeof(mat2x4) == 2 * 4 * sizeof(float) &&
    sizeof(mat3x4) == 3 * 4 * sizeof(float) &&
    sizeof(mat4)   == 4 * 4 * sizeof(float),
    "Required because we treat these as an array of floats.");

} // namespace glsl
