#include "sys.h"
#include "HeavyRectangle.h"

// static transformation matrices.
HeavyRectangle::Transform const HeavyRectangle::xy_to_position = Transform(Eigen::Translation2f{-size, -size}).scale(pos_scale);
HeavyRectangle::Transform const HeavyRectangle::xy_to_uv       = Transform{Transform::Identity()}.scale(uv_scale);
HeavyRectangle::Vector2f const HeavyRectangle::offset[batch_size] = {
  { 0.0f, 0.0f },   // A
  { 0.0f, 1.0f },   // C
  { 1.0f, 0.0f },   // B
  { 1.0f, 0.0f },   // B
  { 0.0f, 1.0f },   // C
  { 1.0f, 1.0f },   // D
};
