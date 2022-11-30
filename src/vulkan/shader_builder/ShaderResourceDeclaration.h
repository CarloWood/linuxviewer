#pragma once

#include "ShaderVariable.h"
#include "ShaderResourceMember.h"
#include "ShaderResourceVariable.h"
#include "BasicType.h"
#include "descriptor/SetIndex.h"
#include <map>
#ifdef CWDEBUG
#include "vk_utils/print_pointer.h"
#endif
#include "debug.h"

namespace vulkan::shader_builder {
class ShaderResourceBase;

class ShaderResourceDeclaration
{
 private:
  static constexpr uint32_t undefined_magic = 0xffffffff;     // Magic number.

  std::string m_glsl_id;
  vk::DescriptorType m_descriptor_type;                 // The descriptor type of this shader resource.
  descriptor::SetIndexHint m_set_index_hint;            // The set index that this resource belongs to.
  // Mutable because we need to set it later.
  mutable uint32_t m_binding{undefined_magic};          // The binding that this resource belongs to. Set to magic value while set_binding wasn't called yet for debugging purposes.
  using shader_resource_variables_container_t = std::vector<ShaderResourceVariable>;
  shader_resource_variables_container_t m_shader_resource_variables;

  ShaderResourceBase const& m_shader_resource;
  vk::ShaderStageFlags m_stage_flags{};

 public:
  ShaderResourceDeclaration(std::string glsl_id, vk::DescriptorType descriptor_type, descriptor::SetIndexHint set_index_hint /*, uint32_t offset, uint32_t array_size = 0*/, ShaderResourceBase const& shader_resource) :
    m_glsl_id(std::move(glsl_id)), m_descriptor_type(descriptor_type), m_set_index_hint(set_index_hint),
    /*m_offset(offset), m_array_size(array_size),*/ m_shader_resource(shader_resource)
  { }

  void add_member(ShaderResourceMember const& member)
  {
    // Only use this to add a single "fake" member (currently only used by Texture).
    ASSERT(m_shader_resource_variables.empty());
    m_shader_resource_variables.emplace_back(member.glsl_id_full(), this);
  }

  void add_members(ShaderResourceMember::container_t const& members)
  {
    for (ShaderResourceMember::container_t::const_iterator member = members.begin(); member != members.end(); ++member)
      m_shader_resource_variables.emplace_back(member->glsl_id_full(), this);
  }

  void used_in(vk::ShaderStageFlagBits shader_stage)
  {
    m_stage_flags |= shader_stage;
  }

  // Not really const, but only called once.
  void set_binding(uint32_t binding) const
  {
    ASSERT(m_binding == undefined_magic);
    m_binding = binding;
  }

  // Accessors.
  std::string const& glsl_id() const { return m_glsl_id; }
  vk::DescriptorType descriptor_type() const { return m_descriptor_type; }
  descriptor::SetIndexHint set_index_hint() const { return m_set_index_hint; }
  uint32_t binding() const { ASSERT(m_binding != undefined_magic); return m_binding; }
  shader_resource_variables_container_t const& shader_resource_variables() const { return m_shader_resource_variables; }
  ShaderResourceBase const& shader_resource() const { return m_shader_resource; }
  vk::ShaderStageFlags stage_flags() const { return m_stage_flags; }
  vk::DescriptorBindingFlags binding_flags() const;
  int32_t descriptor_array_size() const;

#ifdef CWDEBUG
 public:
  void print_on(std::ostream& os) const;

  // Add an ostream inserter for a pointer so that print_pointer is also used
  // for ShaderResourceDeclaration pointers in maps etc.
  friend std::ostream& operator<<(std::ostream& os, ShaderResourceDeclaration const* ptr)
  {
    os << vk_utils::print_pointer(ptr);
    return os;
  }
#endif
};

} // namespace vulkan::shader_builder
