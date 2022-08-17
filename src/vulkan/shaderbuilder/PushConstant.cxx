#include "sys.h"
#include "PushConstant.h"
#include "DeclarationContext.h"
#include "pipeline/ShaderInputData.h"
#include <sstream>
#include <functional>
#include <cstring>

namespace vulkan::shaderbuilder {

std::string PushConstant::name() const
{
  std::ostringstream oss;
  oss << 'v' << std::hash<std::string>{}(prefix()) << '.' << member_name();
  return oss.str();
}

std::string PushConstant::declaration(pipeline::ShaderInputData* shader_input_data) const
{
  std::size_t const hash = std::hash<std::string>{}(prefix());
  auto res = shader_input_data->glsl_id_str_to_declaration_context().try_emplace(prefix(), prefix(), hash);
  // This prefix should already have been inserted by ShaderInputData::add_push_constant.
  //
  //FIXME: fix this comment once the API has stabilized.
  // Call m_pipeline.add_push_constant<MyPushConstant>(), where MyPushConstant is `prefix`
  // in the initialize() virtual function of your FooPipelineCharacteristic (derived from
  // vulkan::pipeline::Characteristic) that uses this push constant.
  ASSERT(!res.second);
  return res.first->second.generate();
}

} // namespace vulkan::shaderbuilder
