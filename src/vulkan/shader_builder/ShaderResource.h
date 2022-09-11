#pragma once

#include "ShaderVariable.h"
#include "ShaderResourceMember.h"
#include "BasicType.h"
#include "descriptor/SetIndex.h"
#include <map>
#include "debug.h"

namespace vulkan::shader_builder {

class ShaderResource
{
 public:
  using members_container_t = std::map<int, ShaderResourceMember>;      // Maps member indexes to their corresponding ShaderResourceMember object.

 private:
  std::string m_glsl_id;
  vk::DescriptorType m_descriptor_type;                         // The descriptor type of this shader resource.
  //FIXME: this should be set to something sensible.
  descriptor::SetIndex m_set_index;                             // The set that this resource belongs to.
//  uint32_t const m_offset;                      // The offset of the variable inside its C++ ENTRY struct.
//  uint32_t const m_array_size;                  // Set to zero when this is not an array.
//  std::map<std::string, ShaderResourceMember> m_members;        // The members of this shader resource.
  members_container_t* m_members_ptr{};

 public:
  ShaderResource(std::string glsl_id, vk::DescriptorType descriptor_type, descriptor::SetIndex set_index /*, uint32_t offset, uint32_t array_size = 0*/) :
    m_glsl_id(std::move(glsl_id)), m_descriptor_type(descriptor_type), m_set_index(set_index) /*, m_offset(offset), m_array_size(array_size)*/ { }

#if 0
  ShaderResourceMember const* add_member(char const* glsl_id_full)
  {
    auto res = m_members.try_emplace(glsl_id_full, glsl_id_full, this);
    ASSERT(res.second);
    return &res.first->second;
  }
#endif

  void add_members(members_container_t* members_ptr)
  {
    m_members_ptr = members_ptr;
    for (members_container_t::iterator member = members_ptr->begin(); member != members_ptr->end(); ++member)
    {
      member->second.set_shader_resource_ptr(this);
    }
  }

  // Accessors.
  std::string const& glsl_id() const { return m_glsl_id; }
  vk::DescriptorType descriptor_type() const { return m_descriptor_type; }
  descriptor::SetIndex set_index() const { return m_set_index; }
  members_container_t* members() const { return m_members_ptr; }

#ifdef CWDEBUG
 public:
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::shader_builder
