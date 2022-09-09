#pragma once

#include <vulkan/vulkan.hpp>
#include <string>

namespace vulkan::pipeline {
class ShaderInputData;
} // namespace vulkan::pipeline

namespace vulkan::shader_builder {
class ShaderVariable;

struct DeclarationContext
{
  virtual ~DeclarationContext() = default;
  virtual std::string generate(vk::ShaderStageFlagBits shader_stage) const = 0;
};

} // namespace vulkan::shader_builder
