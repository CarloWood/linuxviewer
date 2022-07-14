#pragma once

#include <map>
#include <cstdint>

namespace vulkan::shaderbuilder {

struct ShaderVariableLayout;

struct LocationContext
{
  uint32_t next_location = 0;
  std::map<ShaderVariableLayout const*, uint32_t> locations;

  void update_location(ShaderVariableLayout const* shader_variable_layout);
};

} // namespace vulkan::shaderbuilder
