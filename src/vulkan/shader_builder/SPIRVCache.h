#pragma once

#include "ShaderCompiler.h"
#include "ShaderInfo.h"
#include "utils/Badge.h"
#include <vulkan/vulkan.hpp>
#include <cstdint>
#include <string>
#include <vector>
#include <set>
#include "debug.h"

namespace vulkan {

namespace task {
class SynchronousWindow;
} // namespace task

#ifdef CWDEBUG
class Ambifix;
#endif

namespace pipeline {
class AddShaderStage;
} // namespace pipeline

namespace shader_builder {

class ShaderCompiler;

// Objects of this type should be used to keep a cache of compiled SPIR-V code
// when for whatever reason you need to recreate a ShaderModule but don't want
// to recompile the shader.
class SPIRVCache
{
 private:
  std::vector<uint32_t> m_spirv_code;           // Cached, compiled SPIR-V code.

 public:
  // Compile the code in glsl_source_code (as returned from preprocess) and cache it in m_spirv_code.
  void compile(std::string_view glsl_source_code, ShaderCompiler const& compiler, ShaderInfo const& shader_info);

  // Create handle from cached SPIR-V code.
  vk::UniqueShaderModule create_module(
      utils::Badge<vulkan::pipeline::AddShaderStage>, // Use vulkan::pipeline::AddShaderStage::realize_shader instead of this function.
      LogicalDevice const* logical_device
      COMMA_CWDEBUG_ONLY(vulkan::Ambifix const& ambifix)) const;

  // Free resources.
  void reset();
};

} // namespace shader_builder
} // namespace vulkan
