#pragma once

#include "ShaderVariable.h"
#include "BasicType.h"
#include "debug.h"

namespace vulkan::shaderbuilder {

#ifdef CWDEBUG
// This class defines a print_on method.
using utils::has_print_on::operator<<;
#endif

class PushConstant : public ShaderVariable
{
  BasicType m_type;                             // For example vec3.
  char const* const m_glsl_id_str;              // "MyPushConstant::element_name"
  uint32_t const m_array_size;                  // Set to zero when this is not an array.

 public:
  PushConstant(BasicType type, char const* glsl_id_str, uint32_t array_size = 0) : m_type(type), m_glsl_id_str(glsl_id_str), m_array_size(array_size) { }

  std::string prefix() const
  {
    std::string glsl_id_str = m_glsl_id_str;
    return glsl_id_str.substr(0, glsl_id_str.find(':'));
  }

  std::string member_name() const
  {
    return std::strchr(m_glsl_id_str, ':') + 2;
  }

 private:
  // Implement base class interface.
  std::string declaration(pipeline::Pipeline* pipeline) const override;
  char const* glsl_id_str() const override { return m_glsl_id_str; }
  bool is_vertex_attribute() const override { return false; }
  std::string name() const override;

#ifdef CWDEBUG
 public:
  void print_on(std::ostream& os) const { }
#endif
};

} // namespace vulkan::shaderbuilder
