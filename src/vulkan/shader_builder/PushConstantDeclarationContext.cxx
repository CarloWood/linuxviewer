#include "sys.h"
#include "DeclarationContext.h"
#include "ShaderVariable.h"
#include "PushConstant.h"
#include "pipeline/ShaderInputData.h"
#ifdef CWDEBUG
#include "vk_utils/print_flags.h"
#endif
#include "debug.h"

namespace vulkan::shader_builder {

PushConstantDeclarationContext::PushConstantDeclarationContext(std::string prefix, std::size_t hash)
{
  m_header = "layout(push_constant) uniform " + prefix + " {\n";
  m_footer = "} v" + std::to_string(hash) + ";\n";
}

void PushConstantDeclarationContext::glsl_id_str_is_used_in(char const* glsl_id_str, vk::ShaderStageFlagBits shader_stage, PushConstant const* push_constant, pipeline::ShaderInputData* shader_input_data)
{
  DoutEntering(dc::vulkan, "PushConstantDeclarationContext::glsl_id_str_is_used_in(" << glsl_id_str << ", " << shader_stage << ", " << push_constant << " (\"" << push_constant->glsl_id_str() << "\"), " << (void*)shader_input_data << ")");

  uint32_t& minimum_offset = m_minimum_offset.try_emplace(shader_stage, uint32_t{0xffffffff}).first->second;
  uint32_t& maximum_offset = m_maximum_offset.try_emplace(shader_stage, uint32_t{0x0}).first->second;

  // Update offset range of used push constants.
  minimum_offset = std::min(minimum_offset, push_constant->offset());
  maximum_offset = std::max(maximum_offset, push_constant->offset());

  // Make a list of all push constants in this range.
  uint32_t offset_end;
  std::vector<PushConstant const*> push_constants_in_range;
  auto const& push_constants = shader_input_data->glsl_id_str_to_push_constant();
  for (auto&& glsl_id_str_push_constant_pair : push_constants)
  {
    PushConstant const* push_constant = &glsl_id_str_push_constant_pair.second;
    if (minimum_offset <= push_constant->offset() && push_constant->offset() <= maximum_offset)
      push_constants_in_range.push_back(push_constant);
    if (push_constant->offset() == maximum_offset)
      offset_end = push_constant->offset() + push_constant->size();
  }
  // Sort the push constants by offset.
  struct Compare { bool operator()(PushConstant const* pc1, PushConstant const* pc2) { return pc1->offset() < pc2->offset(); } };
  std::sort(push_constants_in_range.begin(), push_constants_in_range.end(), Compare{});

  vk::PushConstantRange push_constant_range = {
    .stageFlags = shader_stage,
    .offset = minimum_offset,
    .size = offset_end - minimum_offset
  };
  // This possibly replaces ranges that were added before, if they have the same stageFlags
  // and push_constant_range completely overlaps them.
  shader_input_data->insert(push_constant_range);

  m_elements.clear();
  bool first = true;
  for (PushConstant const* push_constant : push_constants_in_range)
  {
    BasicType basic_type = push_constant->basic_type();
    std::string member_declaration;
    if (first && minimum_offset > 0)
      member_declaration += "layout(offset = " + std::to_string(minimum_offset) + ") ";
    member_declaration += glsl::type2name(basic_type.scalar_type(), basic_type.rows(), basic_type.cols()) + " " + push_constant->member_name();
    if (push_constant->elements() > 0)
      member_declaration += "[" + std::to_string(push_constant->elements()) + "]";
    member_declaration += std::string{";\t// "} + push_constant->glsl_id_str() + "\n";
    m_elements.push_back(member_declaration);
    first = false;
  }
}

std::string PushConstantDeclarationContext::generate(vk::ShaderStageFlagBits shader_stage) const
{
  DoutEntering(dc::vulkan, "PushConstantDeclarationContext::generate(" << shader_stage << ")");

  // No declaration if this stage doesn't use any push constants.
  if (!m_minimum_offset.contains(shader_stage))
    return {};

  Dout(dc::vulkan, "m_minimum_offset:" << m_minimum_offset.at(shader_stage) << ", m_maximum_offset:" << m_maximum_offset.at(shader_stage));
  std::string result;
#ifdef CWDEBUG
  result = "// " + to_string(shader_stage) + " shader.\n";
#endif
  result += m_header;
  for (auto&& element : m_elements)
    result += "  " + element;
  result += m_footer;
  return result;
}

} // namespace vulkan::shader_builder
