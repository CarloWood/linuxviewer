#pragma once

#include "ShaderVariable.h"
#include "ShaderResourceMember.h"
#include "ShaderResourceVariable.h"
#include "BasicType.h"
#include "../descriptor/SetIndex.h"
#include <map>
#ifdef CWDEBUG
#include "../vk_utils/print_pointer.h"
#include "../vk_utils/print_flags.h"
#endif
#include "debug.h"
#include "cwds/UsageDetector.h"

namespace vulkan::task {
class PipelineFactory;
} // namespace vulkan::task

namespace vulkan::shader_builder {
class ShaderResourceBase;

// This class contains the information that we need to cache in order to
// transfer descriptor set layout binding information between pipeline factories,
// in case one factory compiled a shader module that another also wants to use.
class DescriptorSetLayoutBinding
{
 protected:
  static constexpr uint32_t undefined_magic = 0xffffafff;       // Magic number.

  vk::DescriptorType m_descriptor_type;                 // The descriptor type of this shader resource.
  // Mutable because we need to set it later.
  mutable uint32_t m_binding{undefined_magic};          // The binding that this resource belongs to. Set to magic value while set_binding wasn't called yet for debugging purposes.
  vk::ShaderStageFlags m_stage_flags{};
  int32_t m_descriptor_array_size;
  vk::DescriptorBindingFlags m_binding_flags;
  descriptor::SetIndex m_cached_set_index{};

 public:
  DescriptorSetLayoutBinding(vk::DescriptorType descriptor_type, int32_t descriptor_array_size, vk::DescriptorBindingFlags binding_flags) :
    m_descriptor_type(descriptor_type), m_descriptor_array_size(descriptor_array_size), m_binding_flags(binding_flags) { }

  void set_set_index(descriptor::SetIndex set_index)
  {
    m_cached_set_index = set_index;
  }

  descriptor::SetIndex cached_set_index() const
  {
    ASSERT(!m_cached_set_index.undefined());
    return m_cached_set_index;
  }

  vk::ShaderStageFlags stage_flags() const
  {
    return m_stage_flags;
  }

  int32_t descriptor_array_size() const
  {
    return m_descriptor_array_size;
  }

#if 0
  vk::DescriptorType descriptor_type() const
  {
    return m_descriptor_type;
  }

  uint32_t binding() const
  {
    return m_binding;
  }

  vk::DescriptorBindingFlags binding_flags() const
  {
    return m_binding_flags;
  }
#endif

  void push_back_descriptor_set_layout_binding(task::PipelineFactory* pipeline_factory, descriptor::SetIndexHint set_index_hint) const;

#ifdef CWDEBUG
 public:
  void print_on(std::ostream& os) const;
#endif
};

class ShaderResourceDeclaration : public DescriptorSetLayoutBinding
{
 protected:
  using shader_resource_variables_container_t = std::vector<ShaderResourceVariable>;

 private:
  std::string m_glsl_id;
  descriptor::SetIndexHint m_set_index_hint;                    // The set index that this resource belongs to.
  shader_resource_variables_container_t m_shader_resource_variables;
  ShaderResourceBase const& m_shader_resource;

 public:
  ShaderResourceDeclaration(std::string glsl_id, vk::DescriptorType descriptor_type, descriptor::SetIndexHint set_index_hint /*, uint32_t offset, uint32_t array_size = 0*/, ShaderResourceBase const& shader_resource);

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
  bool has_binding() const { return m_binding != undefined_magic; }
  shader_resource_variables_container_t const& shader_resource_variables() const { return m_shader_resource_variables; }
  ShaderResourceBase const& shader_resource() const { return m_shader_resource; }
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
