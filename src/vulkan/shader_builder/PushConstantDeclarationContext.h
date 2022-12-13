#pragma once

#include "DeclarationContext.h"
#include <vector>
#include <cstddef>
#include <map>

namespace vulkan::shader_builder {

class PushConstantDeclarationContext final : public DeclarationContext
{
 private:
  struct StageData
  {
    uint32_t minimum_offset;            // The minimum offset in the push constant struct of all push constants used in the shader of the key.
    uint32_t maximum_offset;            // The maximum offset in the push constant struct of all push constants used in the shader of the key.
    std::vector<std::string> member_declarations;       // Push constant struct member declarations.
  };

  std::map<vk::ShaderStageFlagBits, StageData> m_stage_data;
  std::string m_header;
  std::string m_footer;

 public:
  PushConstantDeclarationContext(std::string prefix, std::size_t hash);

  void glsl_id_full_is_used_in(char const* glsl_id_full, vk::ShaderStageFlagBits shader_stage, PushConstant const* push_constant, pipeline::AddPushConstant* add_push_constant);

  void add_declarations_for_stage(DeclarationsString& declarations_out, vk::ShaderStageFlagBits shader_stage) const override;
};

} // namespace vulkan::shader_builder
