#pragma once

#include <map>
#include <cstdint>

namespace vulkan::shaderbuilder {

struct ShaderVariableAttribute;

struct LocationContext
{
  uint32_t next_location = 0;
  std::map<ShaderVariableAttribute const*, uint32_t> locations;

  void update_location(ShaderVariableAttribute const* shader_variable_attribute);
};

} // namespace vulkan::shaderbuilder
