#include "sys.h"
#include "ShaderInfo.h"
#include "SynchronousWindow.h"
#include <fstream>

namespace vulkan::shader_builder {

shaderc_shader_kind ShaderInfo::get_shader_kind() const
{
  switch (m_stage)
  {
    case vk::ShaderStageFlagBits::eVertex:
      return shaderc_vertex_shader;
    case vk::ShaderStageFlagBits::eTessellationControl:
      return shaderc_tess_control_shader;
    case vk::ShaderStageFlagBits::eTessellationEvaluation:
      return shaderc_tess_evaluation_shader;
    case vk::ShaderStageFlagBits::eGeometry:
      return shaderc_geometry_shader;
    case vk::ShaderStageFlagBits::eFragment:
      return shaderc_fragment_shader;
    case vk::ShaderStageFlagBits::eCompute:
      return shaderc_compute_shader;
    case vk::ShaderStageFlagBits::eRaygenKHR:
      return shaderc_raygen_shader;
    case vk::ShaderStageFlagBits::eAnyHitKHR:
      return shaderc_anyhit_shader;
    case vk::ShaderStageFlagBits::eClosestHitKHR:
      return shaderc_closesthit_shader;
    case vk::ShaderStageFlagBits::eMissKHR:
      return shaderc_miss_shader;
    case vk::ShaderStageFlagBits::eIntersectionKHR:
      return shaderc_intersection_shader;
    case vk::ShaderStageFlagBits::eCallableKHR:
      return shaderc_callable_shader;
    case vk::ShaderStageFlagBits::eTaskNV:
      return shaderc_task_shader;
    case vk::ShaderStageFlagBits::eMeshNV:
      return shaderc_mesh_shader;
    default:
      THROW_ALERT("It is not supported to pass [STAGEFLAG] as ShaderModule stage flag", AIArgs("[STAGEFLAG]", to_string(m_stage)));
  }
  AI_NEVER_REACHED;
}

ShaderInfo& ShaderInfo::load(std::same_as<std::filesystem::path> auto const& filename)
{
  DoutEntering(dc::vulkan, "ShaderInfo::load(" << filename << ")");

  std::ifstream ifs(filename, std::ios::ate);

  if (ifs.fail())
    THROW_ALERT("Could not open [FILENAME] file!", AIArgs("[FILENAME]", filename));

  std::streampos const file_size = ifs.tellg();
  m_glsl_template_code.resize(file_size);
  ifs.seekg(0, std::ios::beg);
  ifs.read(m_glsl_template_code.data(), file_size);

  ASSERT(ifs.gcount() == file_size);
  ASSERT(ifs.good());

  // Use constructor to set a name, or call set_name(name) before calling this function,
  // if you want to set your own name for this ShaderModule.
  if (m_name.empty())
    m_name = filename.filename();

  return *this;
}

ShaderInfo& ShaderInfo::load(std::string_view source)
{
  // Remove leading white space.
  auto pos = source.find_first_not_of(" \n\t");
  if (pos != std::string_view::npos)
    source.remove_prefix(pos);

  m_glsl_template_code = source;
  return *this;
}

#if 0
vk::UniqueShaderModule ShaderInfo::compile_and_create_module(
    task::SynchronousWindow const* owning_window, ShaderCompiler const& compiler, std::string_view glsl_source_code
    COMMA_CWDEBUG_ONLY(vulkan::AmbifixOwner const& debug_name)) const
{
  DoutEntering(dc::vulkan, "ShaderInfo::compile_and_create(" << owning_window << ", ...)");

  return compiler.compile_and_create({}, owning_window->logical_device(), *this, glsl_source_code
      COMMA_CWDEBUG_ONLY(debug_name));
}
#endif

void ShaderInfo::cleanup_source_code()
{
  m_glsl_template_code.clear();
}

void ShaderInfo::reset()
{
  m_name.clear();
  m_glsl_template_code.clear();
  m_compiler_options = {};
}

#ifdef CWDEBUG
//static
void ShaderInfo::print_source_code_on(std::ostream& os, std::string const& source_code)
{
  std::istringstream glsl_code{source_code};
  std::string line;
  int line_number = 1;
  while (std::getline(glsl_code, line))
  {
    os << "\n" << line_number << "\t" << line;
    ++line_number;
  }
  os << "\n";
}

void ShaderInfo::print_on(std::ostream& os) const
{
  os << '{';
  os << "name:\"" << m_name << "\", code:";
  print_source_code_on(os, m_glsl_template_code);
  os << "stage:" << to_string(m_stage);
  os << "}";
}
#endif

} // namespace vulkan::shader_builder
