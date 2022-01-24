#include "sys.h"
#include "ShaderCompiler.h"
#include "ShaderModule.h"
#include "LogicalDevice.h"
#include "utils/at_scope_end.h"

namespace vulkan::shaderbuilder {

namespace {

struct Compiler
{
  uint32_t const* data_out;
  size_t data_size_out;
  shaderc_compilation_result_t result;

  Compiler(ShaderModule const& shader_module, shaderc_compiler_t compiler, ShaderCompilerOptions options);
  ~Compiler() { shaderc_result_release(result); }
};

Compiler::Compiler(ShaderModule const& shader_module, shaderc_compiler_t compiler, ShaderCompilerOptions options)
{
  // Compile glsl_source.
  std::string_view glsl_source = shader_module.glsl_source();
  // Call load() before calling compile(). A call to reset() clears the source, so load() has to be called again.
  // Call to ShaderModule::compile also clears the source code. In that case call ShaderModule::create(task::SynchronousWindow const*).
  ASSERT(glsl_source.size() > 0);
  auto shader_kind = shader_module.get_shader_kind();
  std::string const& input_file_name = shader_module.name();
  result = shaderc_compile_into_spv(compiler, glsl_source.data(), glsl_source.size(), shader_kind, input_file_name.c_str(), "main", options);
  shaderc_compilation_status compilation_status = shaderc_result_get_compilation_status(result);
  if (compilation_status != shaderc_compilation_status_success)
  {
    auto&& release = at_scope_end([&]{ shaderc_result_release(result); });
    THROW_ALERT("Failed to compile \"[NAME]\" as [KIND]: [ERROR]", AIArgs("[NAME]", input_file_name)("[KIND]", shader_kind)("[ERROR]", shaderc_result_get_error_message(result)));
  }
  data_out = reinterpret_cast<uint32_t const*>(shaderc_result_get_bytes(result));
  data_size_out = shaderc_result_get_length(result);
}

} // namespace

std::vector<uint32_t> ShaderCompiler::compile(utils::Badge<ShaderModule>, ShaderModule const& shader_module, ShaderCompilerOptions options) const
{
  Compiler compiler(shader_module, m_compiler, options);
  // Cache resulting SPIR-V code in a vector.
  return { compiler.data_out, compiler.data_out + compiler.data_size_out / sizeof(uint32_t) };
}

vk::UniqueShaderModule ShaderCompiler::create(utils::Badge<ShaderModule>, vulkan::LogicalDevice const& logical_device, ShaderModule const& shader_module, ShaderCompilerOptions options
    COMMA_CWDEBUG_ONLY(AmbifixOwner const& debug_name)) const
{
  Compiler compiler(shader_module, m_compiler, options);
  // Create the vk::UniqueShaderModule. This is the same as ShaderModule::create() except that it uses what we just compiled instead of the cached SPIR-V code.
  return logical_device.create_shader_module(compiler.data_out, compiler.data_size_out
      COMMA_CWDEBUG_ONLY(debug_name));
}

} // namespace vulkan::shaderbuilder