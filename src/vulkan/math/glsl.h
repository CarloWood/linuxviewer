#pragma once

#include <Eigen/Core>

namespace glsl {

// Allowed types for glsl.
using Float  = float;
using Double = double;
using Bool   = bool;
using Int    = int32_t;
using Uint   = uint32_t;
using Int8   = int8_t;
using Uint8  = uint8_t;
using Int16  = int16_t;
using Uint16 = uint16_t;

// All of these types must be dense float arrays.

using vec2 = Eigen::Vector2f;
using vec3 = Eigen::Vector3f;
using vec4 = Eigen::Vector4f;

using mat2 = Eigen::Matrix2f;
using mat3 = Eigen::Matrix3f;
using mat4 = Eigen::Matrix4f;

static_assert(
    sizeof(vec2) ==     2 * sizeof(float) &&
    sizeof(vec3) ==     3 * sizeof(float) &&
    sizeof(vec4) ==     4 * sizeof(float) &&
    sizeof(mat2) == 2 * 2 * sizeof(float) &&
    sizeof(mat3) == 3 * 3 * sizeof(float) &&
    sizeof(mat4) == 4 * 4 * sizeof(float),
    "Required because we treat these as an array of floats.");

} // namespace glsl
