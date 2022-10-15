#pragma once

#include <vulkan/vulkan.hpp>

namespace vulkan::shader_builder {
class DeclarationsString;

struct DeclarationContext
{
  virtual ~DeclarationContext() = default;
  virtual void add_declarations_for_stage(DeclarationsString& declarations_out, vk::ShaderStageFlagBits shader_stage) const = 0;
};

} // namespace vulkan::shader_builder
