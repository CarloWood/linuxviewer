#pragma once

#include "ShaderVariable.h"
#include "descriptor/SetIndex.h"
#include "debug.h"

namespace vulkan::shaderbuilder {

class ShaderResource final : public ShaderVariable
{
 private:
  vk::DescriptorType m_descriptor_type;         // The descriptor type of this shader resource.
  //FIXME: this should be set to something sensible.
  descriptor::SetIndex m_set{size_t{0}};        // The set that this resource belongs to.
//  uint32_t const m_offset;                      // The offset of the variable inside its C++ ENTRY struct.
//  uint32_t const m_array_size;                  // Set to zero when this is not an array.

 public:
  ShaderResource(char const* glsl_id_str, vk::DescriptorType descriptor_type /*, uint32_t offset, uint32_t array_size = 0*/) :
    ShaderVariable(glsl_id_str), m_descriptor_type(descriptor_type) /*, m_offset(offset), m_array_size(array_size)*/ { }

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

  // Accessors.
  vk::DescriptorType descriptor_type() const { return m_descriptor_type; }
  descriptor::SetIndex set() const { return m_set; }

 private:
  // Implement base class interface.
  DeclarationContext const& is_used_in(vk::ShaderStageFlagBits shader_stage, pipeline::ShaderInputData* shader_input_data) const override;
  std::string name() const override;

#ifdef CWDEBUG
 public:
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::shaderbuilder
