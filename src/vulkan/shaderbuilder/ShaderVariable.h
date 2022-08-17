#pragma once

#include <string>

namespace vulkan::pipeline {
class ShaderInputData;
} // namespace vulkan::pipeline

namespace vulkan::shaderbuilder {

class ShaderVariable
{
 public:
  virtual ~ShaderVariable() = default;
  virtual std::string declaration(pipeline::ShaderInputData* shader_input_data) const = 0;
  virtual char const* glsl_id_str() const = 0;
  virtual bool is_vertex_attribute() const = 0;
  virtual std::string name() const = 0;
};

} // namespace vulkan::shaderbuilder
