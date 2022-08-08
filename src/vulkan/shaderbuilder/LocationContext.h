#pragma once

#include <map>
#include <cstdint>

namespace vulkan::shaderbuilder {

struct ShaderVertexInputAttributeLayout;

struct LocationContext
{
  uint32_t next_location = 0;
  std::map<ShaderVertexInputAttributeLayout const*, uint32_t> locations;

  void update_location(ShaderVertexInputAttributeLayout const* shader_variable_layout);
};

} // namespace vulkan::shaderbuilder
