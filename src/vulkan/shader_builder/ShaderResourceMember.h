#pragma once

#include "ShaderVariable.h"
#include "BasicType.h"
#include "debug.h"

namespace vulkan::shader_builder {
class ShaderResource;

class ShaderResourceMember final : public ShaderVariable
{
 private:
  int m_member_index;                           // Member index (0 for textures).
  ShaderResource* m_shader_resource_ptr{};      // Pointer to the corresponding shader resource, that contains for example SetIndex and descriptor type.
  BasicType const m_basic_type;                 // The glsl basic type of the variable, or underlying basic type in case of an array.
  uint32_t const m_offset;                      // The offset of the variable inside its C++ ENTRY struct.
  uint32_t const m_array_size;                  // Set to zero when this is not an array.

 public:
  // Accessors.
  int member_index() const { return m_member_index; }
  BasicType basic_type() const { return m_basic_type; }
  uint32_t offset() const { return m_offset; }
  uint32_t array_size() const { return m_array_size; }

 public:
  ShaderResourceMember(char const* glsl_id_full, int member_index, BasicType basic_type, uint32_t offset, uint32_t array_size = 0) :
    ShaderVariable(glsl_id_full), m_member_index(member_index), m_basic_type(basic_type), m_offset(offset),
    m_array_size(array_size) { }

  void set_shader_resource_ptr(ShaderResource* shader_resource_ptr)
  {
    m_shader_resource_ptr = shader_resource_ptr;
  }

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

} // namespace vulkan::shader_builder
