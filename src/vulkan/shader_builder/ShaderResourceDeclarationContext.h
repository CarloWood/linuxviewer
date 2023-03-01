#pragma once

#include "DeclarationContext.h"
#include "descriptor/SetIndex.h"
#include "descriptor/SetKey.h"
#include "descriptor/SetIndexHintMap.h"
#include <cstdint>
#include <map>

namespace vulkan::task {
class PipelineFactory;
} // namespace vulkan::task

namespace vulkan::shader_builder {
class ShaderResourceDeclaration;
class ShaderInfoCache;

struct ShaderResourceDeclarationContext final : DeclarationContext
{
 private:
  utils::Vector<uint32_t, descriptor::SetIndexHint> m_next_binding;
  std::map<ShaderResourceDeclaration const*, uint32_t> m_bindings;

  task::PipelineFactory* const m_owning_factory;
  descriptor::SetIndexHintMap const* m_set_index_hint_map;

  int m_changed_generation{0};  // The last AddShaderStage::m_context_changed_generation as passed to generate_descriptor_set_layout_bindings.

 public:
  ShaderResourceDeclarationContext(task::PipelineFactory* owning_factory) : m_owning_factory(owning_factory) { }

  uint32_t binding(ShaderResourceDeclaration const* shader_resource) const
  {
    return m_bindings.at(shader_resource);
  }

  void reserve_binding(descriptor::SetKey descriptor_set_key);
  void update_binding(ShaderResourceDeclaration const* shader_resource);

  void glsl_id_prefix_is_used_in(std::string glsl_id_prefix, vk::ShaderStageFlagBits shader_stage, ShaderResourceDeclaration const* shader_resource, int context_changed_generation);

  void generate_descriptor_set_layout_bindings(vk::ShaderStageFlagBits shader_stage);
  void set_set_index_hint_map(descriptor::SetIndexHintMap const* set_index_hint_map) { m_set_index_hint_map = set_index_hint_map; }
  void add_declarations_for_stage(DeclarationsString& declarations_out, vk::ShaderStageFlagBits shader_stage) const override;

  int changed_generation() const { return m_changed_generation; }

#ifdef CWDEBUG
  std::map<ShaderResourceDeclaration const*, uint32_t> const& bindings() const { return m_bindings; }

  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::shader_builder
