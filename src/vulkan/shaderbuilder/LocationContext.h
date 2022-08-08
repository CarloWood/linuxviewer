#pragma once

#include <map>
#include <cstdint>

namespace vulkan::shaderbuilder {

struct VertexAttribute;

struct LocationContext
{
  uint32_t next_location = 0;
  std::map<VertexAttribute const*, uint32_t> locations;

  void update_location(VertexAttribute const* vertex_attribute);
};

} // namespace vulkan::shaderbuilder
