#pragma once

#include <cstring>
#include <string>

namespace vulkan::shader_builder {

class GlslIdFull
{
 protected:
  char const* m_glsl_id_full;           // "MyPushConstant::element_name"

 public:
  GlslIdFull(char const* glsl_id_full) : m_glsl_id_full(glsl_id_full) { }

  // Accessor.
  char const* glsl_id_full() const { return m_glsl_id_full; }

  std::string prefix() const
  {
    std::string glsl_id_full = m_glsl_id_full;
    return glsl_id_full.substr(0, glsl_id_full.find(':'));
  }

  char const* member_name() const
  {
    return std::strchr(m_glsl_id_full, ':') + 2;
  }
};

} // namespace vulkan::shader_builder
