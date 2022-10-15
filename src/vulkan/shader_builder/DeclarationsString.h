#pragma once

#include "ShaderResourceMember.h"
#include <string>
#include <iosfwd>
#include <set>

namespace vulkan::shader_builder {

class DeclarationsString
{
 private:
  std::set<std::string> m_generated_structs;    // All struct names that were already generated.
  std::string m_content;                        // The generated GLSL declarations.

 public:
  DeclarationsString& operator+=(std::string const& str)
  {
    m_content += str;
    return *this;
  }

  void add_newline()
  {
    m_content += '\n';
  }

  void write_members_to(std::ostream& os, ShaderResourceMember::container_t const& members);

  // Accessors.
  size_t length() const { return m_content.length(); }
  std::string const& content() const { return m_content; }
};

} // namespace vulkan::shader_builder
