#include "sys.h"
#include "DeclarationContext.h"

namespace vulkan::shaderbuilder {

DeclarationContext::DeclarationContext(std::string prefix, std::size_t hash)
{
  m_header = "layout(push_constant) uniform " + prefix + " {\n";
  m_footer = "} v" + std::to_string(hash) + ";\n";
}

void DeclarationContext::add_element(std::string const& element)
{
  m_elements.push_back(element);
}

std::string DeclarationContext::generate()
{
  std::string result = m_header;
  for (auto&& element : m_elements)
    result += "  " + element;
  result += m_footer;
  return result;
}

} // namespace vulkan::shaderbuilder
