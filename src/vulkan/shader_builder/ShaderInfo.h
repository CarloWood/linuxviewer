#ifndef SHADER_BUILDER_SHADER_INFO_H
#define SHADER_BUILDER_SHADER_INFO_H

#include "ShaderCompiler.h"
#include <vulkan/vulkan.hpp>
#include <shaderc/shaderc.h>    // shaderc_shader_kind, shaderc_glsl_infer_from_source
#include <string>
#include <filesystem>
#include <concepts>
#include <atomic>
#ifdef CWDEBUG
#include "debug/vulkan_print_on.h"
#endif

namespace vulkan::task {
class SynchronousWindow;
} // namespace vulkan::task

namespace vulkan::shader_builder {

class ShaderCompilerOptions;

class ShaderInfo
{
 protected:
  vk::ShaderStageFlagBits m_stage;                      // The stage that this shader will be used in. This value is never changed, but can't be const because
                                                        // ShaderInfo needs to be stored in a vector; which requires move semantics (in case the vector needs
                                                        // to reallocate) which requires (move) assignment to work.
  std::string m_name;                                   // Shader name, used for diagnostics.
  std::string m_glsl_template_code;                     // GLSL template source code; loaded with load().
  ShaderCompilerOptions m_compiler_options;             // Compile options to use.
  std::atomic_bool m_compiled{false};                   // Set to true iff the shader was compiled.

 public:
  // Construct an empty ShaderInfo object to be used for the specified stage.
  // A name can be specified at construction or later with set_name. Note that a call to reset() does NOT reset the name.
  ShaderInfo(vk::ShaderStageFlagBits stage, std::string name) : m_stage(stage), m_name(std::move(name)) { }
  ShaderInfo(vk::ShaderStageFlagBits stage, std::string name, ShaderCompilerOptions const& options) : m_stage(stage), m_name(std::move(name)), m_compiler_options(options) { }
  ShaderInfo(vk::ShaderStageFlagBits stage, std::string name, ShaderCompilerOptions&& options) : m_stage(stage), m_name(std::move(name)), m_compiler_options(std::move(options)) { }
  // If no name is provided and the (template) source is loaded from a file then the name is set to the name of the file.
  ShaderInfo(vk::ShaderStageFlagBits stage) : m_stage(stage) { }
  ShaderInfo(vk::ShaderStageFlagBits stage, ShaderCompilerOptions const& options) : m_stage(stage), m_compiler_options(options) { }
  ShaderInfo(vk::ShaderStageFlagBits stage, ShaderCompilerOptions&& options) : m_stage(stage), m_compiler_options(std::move(options)) { }

  // Moving is done during pre-initialization, like from register_shader_templates;
  // therefore we can ignore m_compiled (which will be false anyway).
  ShaderInfo(ShaderInfo&& shader_info) :
    m_stage(shader_info.m_stage),
    m_name(std::move(shader_info.m_name)),
    m_glsl_template_code(std::move(shader_info.m_glsl_template_code)),
    m_compiler_options(std::move(shader_info.m_compiler_options))
  {
    // Paranoia check: see comment above.
    ASSERT(!shader_info.is_compiled());
  }

  // Same.
  ShaderInfo(ShaderInfo const& shader_info) :
    m_stage(shader_info.m_stage),
    m_name(shader_info.m_name),
    m_glsl_template_code(shader_info.m_glsl_template_code),
    m_compiler_options(shader_info.m_compiler_options)
  {
    // Paranoia check: see comment above.
    ASSERT(!shader_info.is_compiled());
  }

  // Set name of this object (for diagnostics).
  ShaderInfo& set_name(std::string name)
  {
    m_name = std::move(name);
    return *this;
  }

  // Open file filename and read its text contents into m_glsl_template_code.
  // Only use this overload when explicitly using a std::filesystem::path.
  ShaderInfo& load(std::same_as<std::filesystem::path> auto const& filename);

  // Load source code from a string.
  ShaderInfo& load(std::string_view source);

  // Set a compiler options object to use.
  ShaderInfo& set_compiler_options(ShaderCompilerOptions const& compiler_options)
  {
    m_compiler_options = compiler_options;
    return *this;
  }

  ShaderInfo& set_compiler_options(ShaderCompilerOptions&& compiler_options)
  {
    m_compiler_options = std::move(compiler_options);
    return *this;
  }

  // Calculate a hash. Only call this function once after full initialization.
  size_t hash()
  {
    m_compiler_options.calculate_hash();
    size_t hash = static_cast<size_t>(m_stage);
    boost::hash_combine(hash, boost::hash<std::string>{}(m_glsl_template_code));
    boost::hash_combine(hash, m_compiler_options.hash());
    return hash;
  }

  // Accessors.
  vk::ShaderStageFlagBits stage() const { return m_stage; }
  std::string const& name() const { return m_name; }
  std::string_view glsl_template_code() const { return m_glsl_template_code; }
  ShaderCompilerOptions const& compiler_options() const { return m_compiler_options; }
  bool is_compiled() const { return m_compiled.load(std::memory_order::acquire); }

  // Called by ShaderCompiler.
  shaderc_shader_kind get_shader_kind() const;

  // Mark this shader as compiled.
  void set_compiled()
  {
    // Paranoia check: we only should compile a shader once.
    ASSERT(!is_compiled());
    m_compiled.store(true, std::memory_order::release);
  }

  // Clean up resources.
  void cleanup_source_code();
  void reset();

#if 0
  // Compile and forget. This does not store the resulting SPIR-V code.
  vk::UniqueShaderModule compile_and_create_module(
      task::SynchronousWindow const* owning_window, ShaderCompiler const& compiler, std::string_view glsl_source_code
      COMMA_CWDEBUG_ONLY(vulkan::AmbifixOwner const& ambifix)) const;
#endif

#ifdef CWDEBUG
  static void print_source_code_on(std::ostream& os, std::string const& source_code);
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::shader_builder

#endif // SHADER_BUILDER_SHADER_INFO_H
