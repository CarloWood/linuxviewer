#pragma once

#include "ShaderVariable.h"
#include "BasicType.h"
#include "descriptor/SetIndex.h"
#include <map>
#include "debug.h"

namespace vulkan::shader_builder {
class ShaderResource;

class ShaderResourceMember final : public ShaderVariable
{
 private:
  int m_member;
  ShaderResource const* m_shader_resource;
  BasicType const m_basic_type;                 // The glsl basic type of the variable, or underlying basic type in case of an array.
  uint32_t const m_offset;                      // The offset of the variable inside its C++ ENTRY struct.
  uint32_t const m_array_size;                  // Set to zero when this is not an array.

 public:
  // Accessors.
  int member() const { return m_member; }
  BasicType basic_type() const { return m_basic_type; }
  uint32_t offset() const { return m_offset; }
  uint32_t array_size() const { return m_array_size; }

 public:
  ShaderResourceMember(char const* glsl_id_str, int member, ShaderResource const* shader_resource, BasicType basic_type, uint32_t offset, uint32_t array_size = 0) :
    ShaderVariable(glsl_id_str), m_member(member), m_shader_resource(shader_resource), m_basic_type(basic_type), m_offset(offset), m_array_size(array_size) { }

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

// ShaderResourceMember is stored in a std::set, which must be ordered by member index.
struct ShaderResourceMemberCompare
{
  bool operator()(ShaderResourceMember const& m1, ShaderResourceMember const& m2) const
  {
    return m1.member() < m2.member();
  }
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
//  std::map<std::string, ShaderResourceMember> m_members;        // The members of this shader resource.

 public:
  ShaderResource(std::string glsl_id_prefix, vk::DescriptorType descriptor_type, descriptor::SetIndex set_index /*, uint32_t offset, uint32_t array_size = 0*/) :
    m_glsl_id_prefix(std::move(glsl_id_prefix)), m_descriptor_type(descriptor_type), m_set_index(set_index) /*, m_offset(offset), m_array_size(array_size)*/ { }

#if 0
  ShaderResourceMember const* add_member(char const* glsl_id_str)
  {
    auto res = m_members.try_emplace(glsl_id_str, glsl_id_str, this);
    ASSERT(res.second);
    return &res.first->second;
  }
#endif

  // Accessors.
  std::string const& glsl_id_prefix() const { return m_glsl_id_prefix; }
  vk::DescriptorType descriptor_type() const { return m_descriptor_type; }
  descriptor::SetIndex set_index() const { return m_set_index; }
//  std::map<std::string, ShaderResourceMember> const& members() const { return m_members; }

#ifdef CWDEBUG
 public:
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::shader_builder
