#pragma once

#include <shaderc/shaderc.h>    // shaderc_shader_kind, shaderc_glsl_infer_from_source
#include <cstdint>
#include <string>
#include <filesystem>
#include <vector>
#ifdef CWDEBUG
#include <iosfwd>
#endif

namespace vulkan::shaderbuilder {

class ShaderModule
{
 private:
  std::string m_name;
  std::string m_glsl_source_code;
  shaderc_shader_kind m_shader_kind;

 public:
  // Construct a ShaderModule with name name.
  ShaderModule(std::string name, shaderc_shader_kind shader_kind = shaderc_glsl_infer_from_source) : m_name(name), m_shader_kind(shader_kind) { }

  // Open file filename and read its text contents into m_glsl_source_code.
  void load_file(std::filesystem::path const& filename);

  // Set what kind of shader this is (can optionally be used if it can not be deduced from the filename extension).
  void set_shader_kind(shaderc_shader_kind shader_kind) { m_shader_kind = shader_kind; }

  // Load source code from a string.
  void load_source(std::string const& source) { m_glsl_source_code = source; }
  void load_source(std::string const& source, shaderc_shader_kind shader_kind) { set_shader_kind(shader_kind); load_source(source); }

  // Compile the code in m_glsl_source_code and return it as SPIR-V code.
  std::vector<uint32_t> compile() const;

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif

 private:
  shaderc_shader_kind filename_to_shader_kind(std::filesystem::path filename, bool force = false) const;
};

} // namespace vulkan::shaderbuilder
