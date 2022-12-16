#pragma once

#include "GlslIdFull.h"
#include <vulkan/vulkan.hpp>
#include <string>

namespace vulkan::pipeline {
class AddPushConstant;
class AddShaderStage;
} // namespace vulkan::pipeline

namespace vulkan::shader_builder {
class  DeclarationContext;

class ShaderVariable : public shader_builder::GlslIdFull
{
 public:
  using shader_builder::GlslIdFull::GlslIdFull;

  virtual ~ShaderVariable() = default;
  virtual DeclarationContext* is_used_in(vk::ShaderStageFlagBits shader_stage, pipeline::AddShaderStage* add_shader_stage) const = 0;
  virtual std::string name() const = 0;
  virtual std::string substitution() const { return name(); }

#ifdef CWDEBUG
  virtual void print_on(std::ostream& os) const = 0;
#endif
};

} // namespace vulkan::shader_builder
