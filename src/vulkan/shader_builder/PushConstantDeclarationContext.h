#pragma once

#include "DeclarationContext.h"
#include <vector>
#include <cstddef>
#include <map>

namespace vulkan::shader_builder {

class PushConstantDeclarationContext final : public DeclarationContext
{
 private:
  std::map<vk::ShaderStageFlagBits, uint32_t> m_minimum_offset;         // The minimum offset in the push constant struct of all push constants used in the shader of the key.
  std::map<vk::ShaderStageFlagBits, uint32_t> m_maximum_offset;         // The maximum offset in the push constant struct of all push constants used in the shader of the key.
  std::string m_header;
  std::vector<std::string> m_elements;
  std::string m_footer;

 public:
  PushConstantDeclarationContext(std::string prefix, std::size_t hash);

  void glsl_id_full_is_used_in(char const* glsl_id_full, vk::ShaderStageFlagBits shader_stage, PushConstant const* push_constant, pipeline::ShaderInputData* shader_input_data);

  void add_declarations_for_stage(DeclarationsString& declarations_out, vk::ShaderStageFlagBits shader_stage) const override;
};

} // namespace vulkan::shader_builder
