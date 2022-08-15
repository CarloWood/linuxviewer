#pragma once

#include <string>
#include <cstddef>
#include <vector>

namespace vulkan::shaderbuilder {

class DeclarationContext
{
  std::string m_header;
  std::vector<std::string> m_elements;
  std::string m_footer;

 public:
  DeclarationContext(std::string prefix, std::size_t hash);

  void add_element(std::string const& element);
  std::string generate();
};

} // namespace vulkan::shaderbuilder
