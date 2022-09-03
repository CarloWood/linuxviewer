#pragma once

#include "DeclarationContext.h"
#include <map>
#include <cstdint>

namespace vulkan::shaderbuilder {

struct VertexAttribute;

struct VertexAttributeDeclarationContext final : DeclarationContext
{
 private:
  uint32_t m_next_location = 0;
  using locations_container_t = std::map<VertexAttribute const*, uint32_t>;
  locations_container_t m_locations;

 public:
  void update_location(VertexAttribute const* vertex_attribute);

  // Accessors.
  uint32_t location(VertexAttribute const* vertex_attribute) const { return m_locations.at(vertex_attribute); }
  locations_container_t const& locations() const { return m_locations; }

  void glsl_id_str_is_used_in(char const* glsl_id_str, vk::ShaderStageFlagBits shader_stage, VertexAttribute const* vertex_attribute, pipeline::ShaderInputData* shader_input_data);

  std::string generate_declaration(vk::ShaderStageFlagBits shader_stage) const override;
};

} // namespace vulkan::shaderbuilder
