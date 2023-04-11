#pragma once

#include "InstanceData.h"
#include <vulkan/shader_builder/VertexShaderInputSet.h>

// Generate 2 InstanceData objects.
//
// Each InstanceData object exist of a single vec4 representing the position
// to put a Square. This class generates the two position vectors
// where x should be 0 and y is -0.5 or 0.5 (z and w equal 0).
//
class TopBottomPositions final : public vulkan::shader_builder::VertexShaderInputSet<InstanceData>
{
 public:
  // Constructor.
  TopBottomPositions() = default;

 private:
  int chunk_count() const override
  {
    return 2;
  }

  // Returns the number of instances.
  int next_batch() override
  {
    return 2;
  }

  // Fill the next batch_size InstanceData objects.
  void create_entry(InstanceData* input_entry_ptr) override
  {
    float y = -0.5f;
    for (int i = 0; i < 2; ++i)
    {
      input_entry_ptr[i].m_position << 0.0f,                // The x coordinate, overwritten with a push constant in the shader.
                                       y,
                                       0.0f,                // The z coordinate.
                                       0.0f;                // Homogeneous coordinates. This is used as an offset (a vector).
      y += 1.0f;
    }
  }
};
