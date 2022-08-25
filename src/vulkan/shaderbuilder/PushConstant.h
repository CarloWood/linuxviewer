#pragma once

#include "ShaderVariable.h"
#include "BasicType.h"
#include "debug/vulkan_print_on.h"
#include "debug.h"

namespace vulkan::shaderbuilder {

class PushConstant final : public ShaderVariable
{
  BasicType m_type;                             // For example vec3.
  char const* const m_glsl_id_str;              // "MyPushConstant::element_name"
  uint32_t const m_offset;                      // The offset of the variable inside its C++ ENTRY struct.
  uint32_t const m_array_size;                  // Set to zero when this is not an array.

 public:
  PushConstant(BasicType type, char const* glsl_id_str, uint32_t offset, uint32_t array_size = 0) : m_type(type), m_glsl_id_str(glsl_id_str), m_offset(offset), m_array_size(array_size) { }

  std::string prefix() const
  {
    std::string glsl_id_str = m_glsl_id_str;
    return glsl_id_str.substr(0, glsl_id_str.find(':'));
  }

  std::string member_name() const
  {
    return std::strchr(m_glsl_id_str, ':') + 2;
  }

  uint32_t offset() const
  {
    return m_offset;
  }

  uint32_t size() const
  {
    uint32_t sz = m_type.size();
    if (m_array_size > 0)
      sz *= m_array_size;
    return sz;
  }

  BasicType basic_type() const
  {
    return m_type;
  }

  uint32_t elements() const
  {
    return m_array_size;
  }

 public:
  // Implement base class interface.
  char const* glsl_id_str() const override { return m_glsl_id_str; }

 private:
  // Implement base class interface.
  DeclarationContext const& is_used_in(vk::ShaderStageFlagBits shader_stage, pipeline::ShaderInputData* shader_input_data) const override;
  std::string name() const override;

#ifdef CWDEBUG
 public:
  void print_on(std::ostream& os) const { }
#endif
};

} // namespace vulkan::shaderbuilder
