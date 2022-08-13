#pragma once

#include <Eigen/Core>
#include "debug.h"

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
    sizeof(vec2)   == sizeof(float[2]) &&
    sizeof(vec3)   == sizeof(float[3]) &&
    sizeof(vec4)   == sizeof(float[4]) &&
    sizeof(mat2)   == sizeof(float[2 * 2]) &&
    sizeof(mat3x2) == sizeof(float[3 * 2]) &&
    sizeof(mat4x2) == sizeof(float[4 * 2]) &&
    sizeof(mat2x3) == sizeof(float[2 * 3]) &&
    sizeof(mat3)   == sizeof(float[3 * 3]) &&
    sizeof(mat4x3) == sizeof(float[4 * 3]) &&
    sizeof(mat2x4) == sizeof(float[2 * 4]) &&
    sizeof(mat3x4) == sizeof(float[3 * 4]) &&
    sizeof(mat4)   == sizeof(float[4 * 4]),
    "Required because we treat these as an array of floats.");

static_assert(
    sizeof(dvec2)   == sizeof(double[2]) &&
    sizeof(dvec3)   == sizeof(double[3]) &&
    sizeof(dvec4)   == sizeof(double[4]) &&
    sizeof(dmat2)   == sizeof(double[2 * 2]) &&
    sizeof(dmat3x2) == sizeof(double[3 * 2]) &&
    sizeof(dmat4x2) == sizeof(double[4 * 2]) &&
    sizeof(dmat2x3) == sizeof(double[2 * 3]) &&
    sizeof(dmat3)   == sizeof(double[3 * 3]) &&
    sizeof(dmat4x3) == sizeof(double[4 * 3]) &&
    sizeof(dmat2x4) == sizeof(double[2 * 4]) &&
    sizeof(dmat3x4) == sizeof(double[3 * 4]) &&
    sizeof(dmat4)   == sizeof(double[4 * 4]),
    "Required because we treat these as an array of double.");

static_assert(
    sizeof(ivec2)   == sizeof(int32_t[2]) &&
    sizeof(ivec3)   == sizeof(int32_t[3]) &&
    sizeof(ivec4)   == sizeof(int32_t[4]),
    "Required because we treat these as an array of int32_t.");

static_assert(
    sizeof(uvec2)   == sizeof(uint32_t[2]) &&
    sizeof(uvec3)   == sizeof(uint32_t[3]) &&
    sizeof(uvec4)   == sizeof(uint32_t[4]),
    "Required because we treat these as an array of uint32_t.");

static_assert(
    sizeof(i8vec2)   == sizeof(int8_t[2]) &&
    sizeof(i8vec3)   == sizeof(int8_t[3]) &&
    sizeof(i8vec4)   == sizeof(int8_t[4]),
    "Required because we treat these as an array of int8_t.");

static_assert(
    sizeof(u8vec2)   == sizeof(uint8_t[2]) &&
    sizeof(u8vec3)   == sizeof(uint8_t[3]) &&
    sizeof(u8vec4)   == sizeof(uint8_t[4]),
    "Required because we treat these as an array of uint8_t.");

static_assert(
    sizeof(i16vec2)   == sizeof(int16_t[2]) &&
    sizeof(i16vec3)   == sizeof(int16_t[3]) &&
    sizeof(i16vec4)   == sizeof(int16_t[4]),
    "Required because we treat these as an array of int16_t.");

static_assert(
    sizeof(u16vec2)   == sizeof(uint16_t[2]) &&
    sizeof(u16vec3)   == sizeof(uint16_t[3]) &&
    sizeof(u16vec4)   == sizeof(uint16_t[4]),
    "Required because we treat these as an array of uint16_t.");

static_assert(
    sizeof(bvec2)   == sizeof(bool[2]) &&
    sizeof(bvec3)   == sizeof(bool[3]) &&
    sizeof(bvec4)   == sizeof(bool[4]),
    "Required because we treat these as an array of bool.");

// If vertex attributes use the same alignment as the 'scalar' standard, then they are
// all aligned as the underlaying type. However this is not the case of Eigen, sometimes
// the types are aligned as a multiple of the alignment of the underlaying type.
constexpr uint32_t cpp_alignment(ScalarIndex scalar_type, int rows, int cols)
{
  Kind const kind = (rows == 1) ? Scalar : (cols == 1) ? Vector : Matrix;
  if (scalar_type == eFloat)
  {
    switch (kind)
    {
      case Scalar:
        return alignof(float);
      case Vector:
        switch (rows)
        {
          case 2:
            return alignof(vec2);
          case 3:
            return alignof(vec3);
          case 4:
            return alignof(vec4);
        }
      case Matrix:
        switch (rows)
        {
          case 2:
            switch (cols)
            {
              case 2:
                return alignof(mat2);
              case 3:
                return alignof(mat3x2);
              case 4:
                return alignof(mat4x2);
            }
          case 3:
            switch (cols)
            {
              case 2:
                return alignof(mat2x3);
              case 3:
                return alignof(mat3);
              case 4:
                return alignof(mat4x3);
            }
          case 4:
            switch (cols)
            {
              case 2:
                return alignof(mat2x4);
              case 3:
                return alignof(mat3x4);
              case 4:
                return alignof(mat4);
            }
        }
    }
  }
  else if (scalar_type == eDouble)
  {
    switch (kind)
    {
      case Scalar:
        return alignof(double);
      case Vector:
        switch (rows)
        {
          case 2:
            return alignof(dvec2);
          case 3:
            return alignof(dvec3);
          case 4:
            return alignof(dvec4);
        }
      case Matrix:
        switch (rows)
        {
          case 2:
            switch (cols)
            {
              case 2:
                return alignof(dmat2);
              case 3:
                return alignof(dmat3x2);
              case 4:
                return alignof(dmat4x2);
            }
          case 3:
            switch (cols)
            {
              case 2:
                return alignof(dmat2x3);
              case 3:
                return alignof(dmat3);
              case 4:
                return alignof(dmat4x3);
            }
          case 4:
            switch (cols)
            {
              case 2:
                return alignof(dmat2x4);
              case 3:
                return alignof(dmat3x4);
              case 4:
                return alignof(dmat4);
            }
        }
    }
  }
  else if (scalar_type == eBool)
  {
    if (kind == Scalar)
      return alignof(bool);
    switch (rows)
    {
      case 2:
        return alignof(bvec2);
      case 3:
        return alignof(bvec3);
      case 4:
        return alignof(bvec4);
    }
  }
  else if (scalar_type == eInt)
  {
    if (kind == Scalar)
      return alignof(int32_t);
    switch (rows)
    {
      case 2:
        return alignof(ivec2);
      case 3:
        return alignof(ivec3);
      case 4:
        return alignof(ivec4);
    }
  }
  else if (scalar_type == eUint)
  {
    if (kind == Scalar)
      return alignof(uint32_t);
    switch (rows)
    {
      case 2:
        return alignof(uvec2);
      case 3:
        return alignof(uvec3);
      case 4:
        return alignof(uvec4);
    }
  }
  else if (scalar_type == eInt8)
  {
    if (kind == Scalar)
      return alignof(int8_t);
    switch (rows)
    {
      case 2:
        return alignof(i8vec2);
      case 3:
        return alignof(i8vec3);
      case 4:
        return alignof(i8vec4);
    }
  }
  else if (scalar_type == eUint8)
  {
    if (kind == Scalar)
      return alignof(uint8_t);
    switch (rows)
    {
      case 2:
        return alignof(u8vec2);
      case 3:
        return alignof(u8vec3);
      case 4:
        return alignof(u8vec4);
    }
  }
  else if (scalar_type == eInt16)
  {
    if (kind == Scalar)
      return alignof(int16_t);
    switch (rows)
    {
      case 2:
        return alignof(i16vec2);
      case 3:
        return alignof(i16vec3);
      case 4:
        return alignof(i16vec4);
    }
  }
  else if (scalar_type == eUint16)
  {
    if (kind == Scalar)
      return alignof(uint16_t);
    switch (rows)
    {
      case 2:
        return alignof(u16vec2);
      case 3:
        return alignof(u16vec3);
      case 4:
        return alignof(u16vec4);
    }
  }
  AI_NEVER_REACHED
}

// Return the GLSL alignment of a scalar, vector or matrix type under the given standard.
constexpr uint32_t alignment(Standard standard, ScalarIndex scalar_type, int rows, int cols)
{
  Kind const kind = (rows == 1) ? Scalar : (cols == 1) ? Vector : Matrix;

  // In the case of vertex_attributes we return the values that a normal C++ struct gives us.
  if (standard == vertex_attributes)
    return cpp_alignment(scalar_type, rows, cols);

  // All scalar types have a minimum size of 4.
  uint32_t const scalar_size = (scalar_type == eDouble) ? 8 : 4;

  // The alignment is equal to the size of the (underlying) scalar type if the standard
  // is `scalar`, and also when the type just is a scalar.
  if (kind == Scalar || standard == scalar)
    return scalar_size;

  // In the case of a vector that is multiplied with 2 or 4 (a vector with 3 rows takes the
  // same space as a vector with 4 rows).
  uint32_t const vector_alignment = scalar_size * ((rows == 2) ? 2 : 4);

  if (kind == Vector)
    return vector_alignment;

  // For matrices round that up to the alignment of a vec4 if the standard is `std140`.
  return (standard == std140) ? std::max(vector_alignment, 16U) : vector_alignment;
}

// Return the GLSL size of a scalar, vector or matrix type under the given standard.
constexpr uint32_t size(Standard standard, ScalarIndex scalar_type, int rows, int cols)
{
  using namespace glsl;
  Kind const kind = (rows == 1) ? Scalar : (cols == 1) ? Vector : Matrix;

  // Non-matrices have the same size as in the standard 'scalar' case.
  // In the case of vertex_attributes we return the values that a normal C++ struct gives us.
  if (kind != Matrix || standard == scalar || standard == vertex_attributes)
    return alignment(standard, scalar_type, 1, 1) * rows * cols;

  // Matrices are layed out as arrays of `cols` column-vectors.
  // The alignment of the matrix is equal to the alignment of one such column-vector, also known as the matrix-stride.
  uint32_t const matrix_stride = alignment(standard, scalar_type, rows, cols);

  // The size of the matrix is simple the size of one of its column-vectors times the number of columns.
  return cols * matrix_stride;
}

// Return the GLSL array_stride of a scalar, vector or matrix type under the given standard.
constexpr uint32_t array_stride(Standard standard, ScalarIndex scalar_type, int rows, int cols)
{
  // The array stride is equal to the largest of alignment and size.
  uint32_t array_stride = std::max(alignment(standard, scalar_type, rows, cols), size(standard, scalar_type, rows, cols));

  // In the case of std140 that must be rounded up to 16.
  return (standard == std140) ? std::max(array_stride, 16U) : array_stride;
}

} // namespace glsl
