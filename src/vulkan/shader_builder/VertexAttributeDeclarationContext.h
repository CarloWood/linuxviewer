#pragma once

#include "DeclarationContext.h"
#include <map>
#include <cstdint>

namespace vulkan::shader_builder {
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

  void glsl_id_full_is_used_in(char const* glsl_id_full, vk::ShaderStageFlagBits shader_stage, VertexAttribute const* vertex_attribute);

  void add_declarations_for_stage(DeclarationsString& declarations_out, vk::ShaderStageFlagBits shader_stage) const override;

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::shader_builder
