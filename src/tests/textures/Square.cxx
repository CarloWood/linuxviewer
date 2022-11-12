#include "sys.h"
#include "Square.h"

// static transformation matrices.
Square::Transform const Square::xy_to_position = Transform(Eigen::Translation2f{-size, -size}).scale(pos_scale);
Square::Transform const Square::xy_to_uv       = Transform{Transform::Identity()}.scale(uv_scale);
Square::Vector2f const Square::offset[batch_size] = {
  { 0.0f, 0.0f },   // A
  { 0.0f, 1.0f },   // C
  { 1.0f, 0.0f },   // B
  { 1.0f, 0.0f },   // B
  { 0.0f, 1.0f },   // C
  { 1.0f, 1.0f },   // D
};
