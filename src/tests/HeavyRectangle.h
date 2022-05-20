#pragma once

#include "VertexData.h"
#include "SampleParameters.h"
#include "vulkan/shaderbuilder/VertexShaderInputSet.h"
#include <Eigen/Geometry>

class HeavyRectangle final : public vulkan::shaderbuilder::VertexShaderInputSet<VertexData>
{
  using Vector2f = Eigen::Vector2f;
  using Transform = Eigen::Affine2f;

  // Each batch exists of two triangles that form a square with coordinates (pos_x, pos_y) where
  // both pos_x and pos_y run from -size to +size in SampleParameters::s_quad_tessellation steps.
  //
  //                                  __ iside = 4
  //                                 /
  //                                v
  //    y=0 ┬─────┬─────┬─────┬─────┐
  //        │     │     │     │     │
  //        │     │     │     │     │  x runs from 0..iside-1
  //      1 ┼─────A─────B─────┼─────┤    y runs from 0..iside-1
  //        │     │   .🮠│     │     │       first triangle: A C B
  //        │     │🮣    │     │     │      second triangle: B C D, both counter clockwise.
  //      2 ┼─────C─────D─────┼─────┤
  //        │     │     │     │     │  Shown are the triangles that belong to x,y = 1,1 (A).
  //        │     │     │     │     │
  //      3 ┼─────┼─────┼─────┼─────┤
  //        │     │     │     │     │
  //        │     │     │     │     │
  //        ├─────┼─────┼─────┼─────┘
  //      x=0     1     2     3
  //        |                       |
  // pos: -size         0          +size
  //  uv:   0                       1
  //
  // At the same time (u, v) correspond with the texture coordinates and run from (0, 0) till (1, 1).
  //
  // Internally we use integer coordinates x and y that run from 0 till SampleParameters::s_quad_tessellation.
  //
  static constexpr int iside       = SampleParameters::s_quad_tessellation;
  static constexpr float size      = 0.12f;
  static constexpr float pos_scale = 2 * size / iside;
  static constexpr float uv_scale  = 1.0f / iside;
  static constexpr int batch_size  = 6;

  static Transform const xy_to_position;
  static Transform const xy_to_uv;
  static Vector2f const offset[6];

  Vector2f xy;                  // Integer coordinates x,y.

 public:
  // Constructor. Start with the square in the upper-left corner.
  HeavyRectangle() : xy(0.f, 0.f) {}

 private:
  // Convert xy to one of the six corners of the two triangles.
  // Here `vertex` runs from 0 till batch_size ([0, batch_size>).
  auto position_at(int vertex)
  {
    glsl::vec4 position;
    position << xy_to_position * (xy + offset[vertex]), 0.0f, 1.0f;     // Homogeneous coordinates.
    return position;
  }

  // Convert xy to one of the four corners of the current square.
  // Here `vertex` runs from 0 till batch_size ([0, batch_size>).
  auto texture_coordinates_at(int vertex)
  {
    glsl::vec2 coords;
    coords << xy_to_uv * (xy + offset[vertex]);
    return coords;
  }

  // Returns the number of vertices.
  int fragment_count() const override
  {
    return batch_size * iside * iside;
  }

  // Returns the number of vertices that a single call to create_entry produce.
  int next_batch() override
  {
    return batch_size;
  }

  // Fill the next batch_size VertexData objects.
  void create_entry(VertexData* input_entry_ptr) override
  {
    for (int vertex = 0; vertex < batch_size; ++vertex)
    {
      input_entry_ptr[vertex].m_position = position_at(vertex);
      input_entry_ptr[vertex].m_texture_coordinates = texture_coordinates_at(vertex);
    }

    // Advance to the next square.
    if (++xy.x() == iside) { xy.x() = 0; ++xy.y(); }
  }
};
