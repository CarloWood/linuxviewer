#pragma once

#include "ShaderVariable.h"
#include "ShaderResourceMember.h"
#include "ShaderResourceVariable.h"
#include "BasicType.h"
#include "descriptor/SetBinding.h"
#include <map>
#ifdef CWDEBUG
#include "vk_utils/print_pointer.h"
#endif
#include "debug.h"

namespace vulkan::shader_builder {
namespace shader_resource {
class Base;
} // namespace shader_resource

class ShaderResourceDeclaration
{
 private:
  std::string m_glsl_id;
  vk::DescriptorType m_descriptor_type;                         // The descriptor type of this shader resource.
  // Mutable because we need to set the binding later.
  mutable descriptor::SetBinding m_set_index_binding_hint;      // The set / binding that this resource belongs to.
//  uint32_t const m_offset;                      // The offset of the variable inside its C++ ENTRY struct.
//  uint32_t const m_array_size;                  // Set to zero when this is not an array.
  using shader_resource_variables_container_t = std::vector<ShaderResourceVariable>;
  shader_resource_variables_container_t m_shader_resource_variables;

  shader_resource::Base const& m_shader_resource;
  vk::ShaderStageFlags m_stage_flags{};

 public:
  ShaderResourceDeclaration(std::string glsl_id, vk::DescriptorType descriptor_type, descriptor::SetIndexHint set_index_hint /*, uint32_t offset, uint32_t array_size = 0*/, shader_resource::Base const& shader_resource) :
    m_glsl_id(std::move(glsl_id)), m_descriptor_type(descriptor_type), m_set_index_binding_hint(set_index_hint),
    /*m_offset(offset), m_array_size(array_size),*/ m_shader_resource(shader_resource)
  { }

  void add_member(ShaderResourceMember const& member)
  {
    // Only use this to add a single "fake" member (currently only used by shader_resource::Texture).
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
    m_set_index_binding_hint.set_binding(binding);
  }

  // Accessors.
  std::string const& glsl_id() const { return m_glsl_id; }
  vk::DescriptorType descriptor_type() const { return m_descriptor_type; }
  descriptor::SetIndexHint set_index_hint() const { return m_set_index_binding_hint.set_index_hint(); }
  uint32_t binding() const { return m_set_index_binding_hint.binding(); }
  shader_resource_variables_container_t const& shader_resource_variables() const { return m_shader_resource_variables; }
  shader_resource::Base const& shader_resource() const { return m_shader_resource; }
  vk::ShaderStageFlags stage_flags() const { return m_stage_flags; }

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
