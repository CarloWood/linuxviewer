#pragma once

#include "shader_builder/GlslIdFull.h"
#include <vulkan/vulkan.hpp>
#include <string>

namespace vulkan::pipeline {
class ShaderInputData;
} // namespace vulkan::pipeline

namespace vulkan::shader_builder {
class  DeclarationContext;

class ShaderVariable : public shader_builder::GlslIdFull
{
 public:
  using shader_builder::GlslIdFull::GlslIdFull;

  virtual ~ShaderVariable() = default;
  virtual DeclarationContext* is_used_in(vk::ShaderStageFlagBits shader_stage, pipeline::ShaderInputData* shader_input_data) const = 0;
  virtual std::string name() const = 0;

#ifdef CWDEBUG
  virtual void print_on(std::ostream& os) const = 0;
#endif
};

} // namespace vulkan::shader_builder
