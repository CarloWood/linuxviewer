#pragma once

#include "ShaderVariable.h"
#include "descriptor/SetIndex.h"
#include "debug.h"

namespace vulkan::shaderbuilder {
class ShaderResource;

class ShaderResourceMember final : public ShaderVariable
{
 private:
  ShaderResource const* m_shader_resource;

 public:
  ShaderResourceMember(char const* glsl_id_str, ShaderResource const* shader_resource) :
    ShaderVariable(glsl_id_str), m_shader_resource(shader_resource) { }

  // Accessors.
#if 0
  uint32_t offset() const
  {
    return m_offset;
  }

  uint32_t elements() const
  {
    return m_array_size;
  }
#endif

#if 0
  uint32_t size() const
  {
    uint32_t sz = m_type.size();
    if (m_array_size > 0)
      sz *= m_array_size;
    return sz;
  }
#endif

 private:
  // Implement base class interface.
  DeclarationContext const& is_used_in(vk::ShaderStageFlagBits shader_stage, pipeline::ShaderInputData* shader_input_data) const override;
  std::string name() const override;

#ifdef CWDEBUG
 public:
  void print_on(std::ostream& os) const;
#endif
};

class ShaderResource
{
 private:
  std::string m_glsl_id_prefix;
  vk::DescriptorType m_descriptor_type;                         // The descriptor type of this shader resource.
  //FIXME: this should be set to something sensible.
  descriptor::SetIndex m_set_index;                             // The set that this resource belongs to.
//  uint32_t const m_offset;                      // The offset of the variable inside its C++ ENTRY struct.
//  uint32_t const m_array_size;                  // Set to zero when this is not an array.
  std::map<std::string, ShaderResourceMember> m_members;        // The members of this shader resource.

 public:
  ShaderResource(std::string glsl_id_prefix, vk::DescriptorType descriptor_type, descriptor::SetIndex set_index /*, uint32_t offset, uint32_t array_size = 0*/) :
    m_glsl_id_prefix(std::move(glsl_id_prefix)), m_descriptor_type(descriptor_type), m_set_index(set_index) /*, m_offset(offset), m_array_size(array_size)*/ { }

  ShaderResourceMember const* add_member(char const* glsl_id_str)
  {
    auto res = m_members.try_emplace(glsl_id_str, glsl_id_str, this);
    ASSERT(res.second);
    return &res.first->second;
  }

  // Accessors.
  std::string const& glsl_id_prefix() const { return m_glsl_id_prefix; }
  vk::DescriptorType descriptor_type() const { return m_descriptor_type; }
  descriptor::SetIndex set_index() const { return m_set_index; }
  std::map<std::string, ShaderResourceMember> const& members() const { return m_members; }

#ifdef CWDEBUG
 public:
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::shaderbuilder
