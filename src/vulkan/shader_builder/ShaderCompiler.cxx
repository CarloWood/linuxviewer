#include "sys.h"
#include "ShaderCompiler.h"
#include "SPIRVCache.h"
#include "LogicalDevice.h"
#include "utils/at_scope_end.h"

namespace vulkan::shader_builder {

namespace {

struct Compiler
{
  uint32_t const* data_out;
  size_t data_size_out;
  shaderc_compilation_result_t result;

  Compiler(ShaderInfo const& shader_info, std::string_view glsl_source_code, shaderc_compiler_t compiler);
  ~Compiler() { shaderc_result_release(result); }
};

// Compile glsl_source_code.
Compiler::Compiler(ShaderInfo const& shader_info, std::string_view glsl_source_code, shaderc_compiler_t compiler)
{
  Dout(dc::vulkan, "Compiling:" << utils::print_using(std::string{glsl_source_code}, &ShaderInfo::print_source_code_on));

  // Can this still fail?
  ASSERT(glsl_source_code.size() > 0);
  auto shader_kind = shader_info.get_shader_kind();
  std::string const& input_file_name = shader_info.name();
  result = shaderc_compile_into_spv(compiler, glsl_source_code.data(), glsl_source_code.size(),
      shader_kind, input_file_name.c_str(), "main", shader_info.compiler_options());
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

std::vector<uint32_t> ShaderCompiler::compile(utils::Badge<SPIRVCache>, ShaderInfo const& shader_info, std::string_view glsl_source_code) const
{
  Compiler compiler(shader_info, glsl_source_code, m_compiler);
  // Cache resulting SPIR-V code in a vector.
  return { compiler.data_out, compiler.data_out + compiler.data_size_out / sizeof(uint32_t) };
}

vk::UniqueShaderModule ShaderCompiler::compile_and_create(utils::Badge<ShaderInfo>,
    vulkan::LogicalDevice const* logical_device, ShaderInfo const& shader_info, std::string_view glsl_source_code
    COMMA_CWDEBUG_ONLY(AmbifixOwner const& debug_name)) const
{
  Compiler compiler(shader_info, glsl_source_code, m_compiler);
  // Create the vk::UniqueShaderModule. This is the same as SPIRVCache::create_module() except that it uses what we just compiled instead of the cached SPIR-V code.
  return logical_device->create_shader_module(compiler.data_out, compiler.data_size_out
      COMMA_CWDEBUG_ONLY(debug_name));
}

} // namespace vulkan::shader_builder
