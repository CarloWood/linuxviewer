#pragma once

#include "ShaderCompiler.h"
#include <vulkan/vulkan.hpp>
#include <shaderc/shaderc.h>    // shaderc_shader_kind, shaderc_glsl_infer_from_source
#include <cstdint>
#include <string>
#include <filesystem>
#include <vector>
#include <concepts>
#include "debug.h"
#ifdef CWDEBUG
#include <iosfwd>
#endif

namespace task {
class SynchronousWindow;
} // namespace task

namespace vulkan::shaderbuilder {

class ShaderModule
{
 private:
  std::string m_name;
  std::string m_glsl_source_code;
  shaderc_shader_kind m_shader_kind;
  std::vector<uint32_t> m_spirv_code;

 public:
  // Construct an empty ShaderModule object.
  ShaderModule() : m_shader_kind(shaderc_glsl_infer_from_source) { }

  // Construct with name and kind.
  ShaderModule(std::string name, shaderc_shader_kind shader_kind = shaderc_glsl_infer_from_source)
  {
    m_name = std::move(name);
    m_shader_kind = shader_kind;
    if (shader_kind == shaderc_glsl_infer_from_source)
      m_shader_kind = filename_to_shader_kind(m_name);
  }

  // Set name of this object (for diagnostics) and the kind of shader that this is.
  // Can (optionally) be used if it can not be deduced from the filename extension.
  ShaderModule& set_name(std::string name, shaderc_shader_kind shader_kind = shaderc_glsl_infer_from_source)
  {
    // Only set name and shader kind once. Call reset() if you want to cleanup/reuse a ShaderModule object.
    ASSERT(m_name.empty() && m_shader_kind == shaderc_glsl_infer_from_source);
    m_name = std::move(name);
    m_shader_kind = shader_kind;
    if (shader_kind == shaderc_glsl_infer_from_source)
      m_shader_kind = filename_to_shader_kind(m_name);
    return *this;
  }

  // Open file filename and read its text contents into m_glsl_source_code.
  // Only use this overload when explicitly using a std::filesystem::path.
  ShaderModule& load(std::same_as<std::filesystem::path> auto const& filename);

  // Load source code from a string.
  ShaderModule& load(std::string_view source, shaderc_shader_kind shader_kind = shaderc_glsl_infer_from_source)
  {
    if (shader_kind != shaderc_glsl_infer_from_source)
      m_shader_kind = shader_kind;
    m_glsl_source_code = source;
    return *this;
  }

  // Compile the code in m_glsl_source_code and cache it in m_spirv_code.
  void compile(ShaderCompiler const& compiler, ShaderCompilerOptions const& options);

  // Compile and forget.
  vk::UniqueShaderModule create(task::SynchronousWindow const* owning_window, ShaderCompiler const& compiler, ShaderCompilerOptions const& options) const;

  // Create handle from cached SPIR-V code.
  // Use task::SynchronousWindow::create(shader_module) instead of this function.
  vk::UniqueShaderModule create(utils::Badge<task::SynchronousWindow>, task::SynchronousWindow const* owning_window) const;

  // Free resources.
  void reset();

  // Accessors.
  std::string_view glsl_source() const { return m_glsl_source_code; }
  shaderc_shader_kind shader_kind() const { return m_shader_kind; }
  std::string const& name() const { return m_name; }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif

 private:
  shaderc_shader_kind filename_to_shader_kind(std::filesystem::path filename, bool force = false) const;
};

} // namespace vulkan::shaderbuilder
