#pragma once

#include "DeclarationContext.h"
#include <map>
#include <cstdint>

namespace vulkan::shaderbuilder {

struct VertexAttribute;

struct VertexAttributeDeclarationContext final : DeclarationContext
{
  uint32_t next_location = 0;
  std::map<VertexAttribute const*, uint32_t> locations;

  uint32_t location(VertexAttribute const* vertex_attribute) const
  {
    return locations.at(vertex_attribute);
  }

  void update_location(VertexAttribute const* vertex_attribute);

  void glsl_id_str_is_used_in(char const* glsl_id_str, vk::ShaderStageFlagBits shader_stage, ShaderVariable const* shader_variable, pipeline::ShaderInputData* shader_input_data) override;
  std::string generate_declaration(vk::ShaderStageFlagBits shader_stage) const override;
};

} // namespace vulkan::shaderbuilder
