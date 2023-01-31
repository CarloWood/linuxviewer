#include "sys.h"
#include "DeclarationContext.h"
#include "ShaderVariable.h"
#include "PushConstant.h"
#include "pipeline/AddPushConstant.h"
#include "shader_builder/DeclarationsString.h"
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

// Called from PushConstant::is_used_in.
void PushConstantDeclarationContext::glsl_id_full_is_used_in(char const* glsl_id_full, std::type_index type_index, vk::ShaderStageFlagBits shader_stage, PushConstant const* push_constant, pipeline::AddPushConstant* add_push_constant)
{
  DoutEntering(dc::vulkan, "PushConstantDeclarationContext::glsl_id_full_is_used_in(" << glsl_id_full << ", " << shader_stage << ", " << push_constant << " (\"" << push_constant->glsl_id_full() << "\"), " << (void*)add_push_constant << ")");

  auto ibp = m_stage_data.try_emplace(shader_stage, 0xffffffff, 0x0);
  uint32_t& minimum_offset = ibp.first->second.minimum_offset;
  uint32_t& maximum_offset = ibp.first->second.maximum_offset;
  std::vector<std::string>& member_declarations = ibp.first->second.member_declarations;

  // Update offset range of used push constants.
  minimum_offset = std::min(minimum_offset, push_constant->offset());
  maximum_offset = std::max(maximum_offset, push_constant->offset());

  // Make a list of all push constants in this range.
  uint32_t offset_end;
  std::vector<PushConstant const*> push_constants_in_range;
  auto const& push_constants = add_push_constant->glsl_id_full_to_push_constant();
  for (auto&& glsl_id_full_push_constant_pair : push_constants)
  {
    PushConstant const* push_constant = &glsl_id_full_push_constant_pair.second;
    if (minimum_offset <= push_constant->offset() && push_constant->offset() <= maximum_offset)
      push_constants_in_range.push_back(push_constant);
    if (push_constant->offset() == maximum_offset)
      offset_end = push_constant->offset() + push_constant->size();
  }
  // Sort the push constants by offset.
  struct Compare { bool operator()(PushConstant const* pc1, PushConstant const* pc2) { return pc1->offset() < pc2->offset(); } };
  std::sort(push_constants_in_range.begin(), push_constants_in_range.end(), Compare{});

  vulkan::PushConstantRange push_constant_range{type_index, shader_stage, minimum_offset, offset_end - minimum_offset};
  // This possibly replaces ranges that were added before, if they have the same stageFlags
  // and push_constant_range completely overlaps them.
  add_push_constant->insert_push_constant_range(push_constant_range);

  // (Re)generate member_declarations.
  member_declarations.clear();
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
    member_declaration += std::string{";\t// "} + push_constant->glsl_id_full() + "\n";
    member_declarations.push_back(member_declaration);
    first = false;
  }
}

void PushConstantDeclarationContext::add_declarations_for_stage(DeclarationsString& declarations_out, vk::ShaderStageFlagBits shader_stage) const
{
  DoutEntering(dc::vulkan, "PushConstantDeclarationContext::generate(declarations_out, " << shader_stage << ")");

  // No declaration if this stage uses any push constants.
  if (!m_stage_data.contains(shader_stage))
    return;

  Dout(dc::vulkan, "minimum_offset:" << m_stage_data.at(shader_stage).minimum_offset << ", maximum_offset:" << m_stage_data.at(shader_stage).maximum_offset);
#ifdef CWDEBUG
  declarations_out += "// " + to_string(shader_stage) + " shader.\n";
#endif
  declarations_out += m_header;
  for (auto&& member_declaration : m_stage_data.at(shader_stage).member_declarations)
    declarations_out += "  " + member_declaration;
  declarations_out += m_footer;
}

} // namespace vulkan::shader_builder
