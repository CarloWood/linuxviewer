#pragma once

#include "utils/AIAlert.h"
#include "utils/Badge.h"
#include <shaderc/shaderc.h>
#include <vulkan/vulkan.hpp>
#include <string_view>
#include <vector>
#include "debug.h"

namespace vulkan {
class LogicalDevice;
#ifdef CWDEBUG
class AmbifixOwner;
#endif
} // namespace vulkan

namespace vulkan::shaderbuilder {

class SPIRVCache;
class ShaderInfo;
namespace {
struct Compiler;
} // namespace

class ShaderCompilerOptions
{
 private:
  shaderc_compile_options_t m_options;

 public:
  // Default constructor.
  ShaderCompilerOptions()
  {
    m_options = shaderc_compile_options_initialize();
    if (m_options == nullptr)
      THROW_ALERT("Error initializing shader compiler options");
  }

  // Copy constructor.
  ShaderCompilerOptions(ShaderCompilerOptions const& compiler_options) : m_options(shaderc_compile_options_clone(compiler_options.m_options))
  {
  }

  // Move constructor.
  ShaderCompilerOptions(ShaderCompilerOptions&& compiler_options) : m_options(nullptr)
  {
    std::swap(m_options, compiler_options.m_options);
  }

  ShaderCompilerOptions& operator=(ShaderCompilerOptions const& compiler_options)
  {
    shaderc_compile_options_release(m_options);
    m_options = shaderc_compile_options_clone(compiler_options.m_options);
    return *this;
  }

  ShaderCompilerOptions& operator=(ShaderCompilerOptions&& compiler_options)
  {
    shaderc_compile_options_release(m_options);
    m_options = compiler_options.m_options;
    compiler_options.m_options = nullptr;
    return *this;
  }

  ~ShaderCompilerOptions()
  {
    shaderc_compile_options_release(m_options);
  }

  // Adds a predefined macro to the compilation options. This has the same
  // effect as passing -Dname=value to the command-line compiler.  If value
  // is NULL, it has the same effect as passing -Dname to the command-line
  // compiler. If a macro definition with the same name has previously been
  // added, the value is replaced with the new value. The macro name and
  // value are passed in with char pointers, which point to their data, and
  // the lengths of their data. The strings that the name and value pointers
  // point to must remain valid for the duration of the call, but can be
  // modified or deleted after this function has returned. In case of adding
  // a valueless macro, the value argument should be a null pointer or the
  // value_length should be 0u.
  ShaderCompilerOptions& add_macro_definition(char const* name, size_t name_length, char const* value, size_t value_length)
  {
    shaderc_compile_options_add_macro_definition(m_options, name, name_length, value, value_length);
    return *this;
  }

  // Sets the compiler mode to generate debug information in the output.
  ShaderCompilerOptions& set_generate_debug_info()
  {
    shaderc_compile_options_set_generate_debug_info(m_options);
    return *this;
  }

  // Sets the compiler optimization level to the given level. Only the last one
  // takes effect if multiple calls of this function exist.
  ShaderCompilerOptions& set_optimization_level(shaderc_optimization_level level)
  {
    shaderc_compile_options_set_optimization_level(m_options, level);
    return *this;
  }

  // Forces the GLSL language version and profile to a given pair. The version
  // number is the same as would appear in the #version annotation in the source.
  // Version and profile specified here overrides the #version annotation in the
  // source. Use profile: 'shaderc_profile_none' for GLSL versions that do not
  // define profiles, e.g. versions below 150.
  ShaderCompilerOptions& set_forced_version_profile(int version, shaderc_profile profile)
  {
    shaderc_compile_options_set_forced_version_profile(m_options, version, profile);
    return *this;
  }

  // Sets the compiler mode to suppress warnings, overriding warnings-as-errors
  // mode. When both suppress-warnings and warnings-as-errors modes are
  // turned on, warning messages will be inhibited, and will not be emitted
  // as error messages.
  ShaderCompilerOptions& set_suppress_warnings()
  {
    shaderc_compile_options_set_suppress_warnings(m_options);
    return *this;
  }

  // Sets the target shader environment, affecting which warnings or errors will
  // be issued.  The version will be for distinguishing between different versions
  // of the target environment.  The version value should be either 0 or
  // a value listed in shaderc_env_version.  The 0 value maps to Vulkan 1.0 if
  // |target| is Vulkan, and it maps to OpenGL 4.5 if |target| is OpenGL.
  ShaderCompilerOptions& set_target_env(shaderc_target_env target, uint32_t version)
  {
    shaderc_compile_options_set_target_env(m_options, target, version);
    return *this;
  }

  // Sets the target SPIR-V version. The generated module will use this version
  // of SPIR-V.  Each target environment determines what versions of SPIR-V
  // it can consume.  Defaults to the highest version of SPIR-V 1.0 which is
  // required to be supported by the target environment.  E.g. Default to SPIR-V
  // 1.0 for Vulkan 1.0 and SPIR-V 1.3 for Vulkan 1.1.
  ShaderCompilerOptions& set_target_spirv(shaderc_spirv_version version)
  {
    shaderc_compile_options_set_target_spirv(m_options, version);
    return *this;
  }

  // Sets the compiler mode to treat all warnings as errors. Note the
  // suppress-warnings mode overrides this option, i.e. if both
  // warning-as-errors and suppress-warnings modes are set, warnings will not
  // be emitted as error messages.
  ShaderCompilerOptions& set_warnings_as_errors()
  {
    shaderc_compile_options_set_warnings_as_errors(m_options);
    return *this;
  }

  // Sets a resource limit.
  ShaderCompilerOptions& set_limit(shaderc_limit limit, int value)
  {
    shaderc_compile_options_set_limit(m_options, limit, value);
    return *this;
  }

  // Sets whether the compiler should automatically assign bindings to uniforms
  // that aren't already explicitly bound in the shader source.
  ShaderCompilerOptions& set_auto_bind_uniforms(bool auto_bind)
  {
    shaderc_compile_options_set_auto_bind_uniforms(m_options, auto_bind);
    return *this;
  }

  // Sets whether the compiler should automatically remove sampler variables
  // and convert image variables to combined image-sampler variables.
  ShaderCompilerOptions& set_auto_combined_image_sampler(bool upgrade)
  {
    shaderc_compile_options_set_auto_combined_image_sampler(m_options, upgrade);
    return *this;
  }

  // Sets the base binding number used for for a uniform resource type when
  // automatically assigning bindings.  For GLSL compilation, sets the lowest
  // automatically assigned number.
  ShaderCompilerOptions& set_binding_base(shaderc_uniform_kind kind, uint32_t base)
  {
    shaderc_compile_options_set_binding_base(m_options, kind, base);
    return *this;
  }

  // Like shaderc_compile_options_set_binding_base, but only takes effect when
  // compiling a given shader stage.  The stage is assumed to be one of vertex,
  // fragment, tessellation evaluation, tesselation control, geometry, or compute.
  ShaderCompilerOptions& set_binding_base_for_stage(shaderc_shader_kind shader_kind, shaderc_uniform_kind kind, uint32_t base)
  {
    shaderc_compile_options_set_binding_base_for_stage(m_options, shader_kind, kind, base);
    return *this;
  }

  // Sets whether the compiler should automatically assign locations to
  // uniform variables that don't have explicit locations in the shader source.
  ShaderCompilerOptions& set_auto_map_locations(bool auto_map)
  {
    shaderc_compile_options_set_auto_map_locations(m_options, auto_map);
    return *this;
  }

  // Sets whether the compiler should invert position.Y output in vertex shader.
  ShaderCompilerOptions& set_invert_y(bool enable)
  {
    shaderc_compile_options_set_invert_y(m_options, enable);
    return *this;
  }

  // Sets whether the compiler generates code for max and min builtins which,
  // if given a NaN operand, will return the other operand. Similarly, the clamp
  // builtin will favour the non-NaN operands, as if clamp were implemented
  // as a composition of max and min.
  ShaderCompilerOptions& set_nan_clamp(bool enable)
  {
    shaderc_compile_options_set_nan_clamp(m_options, enable);
    return *this;
  }

 private:
  // Accessor.
  friend Compiler;
  operator shaderc_compile_options_t() const { return m_options; }
};

class ShaderCompiler
{
  shaderc_compiler_t m_compiler;

 public:
  ShaderCompiler()
  {
    m_compiler = shaderc_compiler_initialize();
    if (m_compiler == nullptr)
      THROW_ALERT("Error initializing shader compiler");
  }

  // Move constructor.
  ShaderCompiler(ShaderCompiler&& compiler) : m_compiler(nullptr)
  {
    std::swap(m_compiler, compiler.m_compiler);
  }

  ~ShaderCompiler()
  {
    shaderc_compiler_release(m_compiler);
  }

  // Calls to compile are thread-safe.
  std::vector<uint32_t> compile(utils::Badge<SPIRVCache>, ShaderInfo const& shader_info, std::string_view glsl_source_code) const;

  vk::UniqueShaderModule compile_and_create(utils::Badge<ShaderInfo>, vulkan::LogicalDevice const* logical_device, ShaderInfo const& shader_info, std::string_view glsl_source_code
      COMMA_CWDEBUG_ONLY(AmbifixOwner const& debug_name)) const;
};

} // namespace vulkan::shaderbuilder
