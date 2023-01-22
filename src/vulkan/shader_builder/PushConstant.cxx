#include "sys.h"
#include "PushConstant.h"
#include "DeclarationContext.h"
#include "pipeline/AddPushConstant.h"
#include "pipeline/AddShaderStage.h"
#include "vk_utils/print_flags.h"
#include <sstream>
#include <functional>
#include <cstring>

namespace vulkan::shader_builder {

std::string PushConstant::name() const
{
  std::ostringstream oss;
  oss << 'v' << std::hash<std::string>{}(prefix()) << '.' << member_name();
  return oss.str();
}

// Called from AddShaderStage::preprocess1.
DeclarationContext* PushConstant::is_used_in(vk::ShaderStageFlagBits shader_stage, pipeline::AddShaderStage* add_shader_stage) const
{
  DoutEntering(dc::vulkan, "PushConstant::is_used_in(" << shader_stage << ", " << add_shader_stage << ") [" << this << "]");
  pipeline::AddPushConstant* add_push_constant = add_shader_stage->convert_to_add_push_constant();

  auto push_constant_declaration_context_iter = add_push_constant->glsl_id_full_to_push_constant_declaration_context({}).find(prefix());
  // This prefix should already have been inserted by AddPushConstant::add_push_constant.
  //
  // Call add_push_constant<MyPushConstant>(), where MyPushConstant is `prefix()`
  // in the initialize() virtual function of your FooPipelineCharacteristic (derived from
  // vulkan::pipeline::Characteristic[Range] and pipeline::AddPushConstant) that uses this push constant.
  ASSERT(push_constant_declaration_context_iter != add_push_constant->glsl_id_full_to_push_constant_declaration_context({}).end());
  PushConstantDeclarationContext* push_constant_declaration_context = push_constant_declaration_context_iter->second.get();

  // Register that this push constant is being used.
  push_constant_declaration_context->glsl_id_full_is_used_in(glsl_id_full(), shader_stage, this, add_push_constant);

  // Return the declaration context.
  return push_constant_declaration_context;
}

#ifdef CWDEBUG
void PushConstant::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_type:" << m_type <<
      ", m_offset:" << m_offset <<
      ", m_array_size:" << m_array_size;
  os << '}';
}
#endif

} // namespace vulkan::shader_builder
