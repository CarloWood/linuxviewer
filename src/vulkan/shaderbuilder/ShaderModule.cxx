#include "sys.h"
#include "ShaderModule.h"
#include "LogicalDevice.h"
#include "SynchronousWindow.h"
#include "utils/AIAlert.h"
#include "utils/at_scope_end.h"
#include <shaderc/shaderc.hpp>
#include <magic_enum.hpp>
#include "debug.h"
#ifdef CWDEBUG
#include <sstream>
#endif

std::ostream& operator<<(std::ostream& os, shaderc_shader_kind kind)
{
  return os << magic_enum::enum_name(kind);
}

namespace vulkan::shaderbuilder {

#if 0
shaderc_shader_kind ShaderModule::filename_to_shader_kind(std::filesystem::path filename, bool force) const
{
  if (filename.extension() == ".glsl")
    filename = filename.stem();
  auto ext = filename.extension();
  if (ext == ".vert")
    return force ? shaderc_vertex_shader : shaderc_glsl_default_vertex_shader;
  if (ext == ".frag")
    return force ? shaderc_fragment_shader : shaderc_glsl_default_fragment_shader;
  if (ext == ".comp")
    return force ? shaderc_compute_shader : shaderc_glsl_default_compute_shader;
  if (ext == ".geom")
    return force ? shaderc_geometry_shader : shaderc_glsl_default_geometry_shader;
  if (ext == ".tesc")
    return force ? shaderc_tess_control_shader : shaderc_glsl_default_tess_control_shader;
  if (ext == ".tese")
    return force ? shaderc_tess_evaluation_shader : shaderc_glsl_default_tess_evaluation_shader;
  if (ext == ".mesh")
    return force ? shaderc_mesh_shader : shaderc_glsl_default_mesh_shader;
  if (ext == ".task")
    return force ? shaderc_task_shader : shaderc_glsl_default_task_shader;
  if (ext == ".rgen")
    return force ? shaderc_raygen_shader : shaderc_glsl_default_raygen_shader;
  if (ext == ".rchit")
    return force ? shaderc_closesthit_shader : shaderc_glsl_default_closesthit_shader;
  if (ext == ".rmiss")
    return force ? shaderc_miss_shader : shaderc_glsl_default_miss_shader;
  if (ext == ".rahit")
    return force ? shaderc_anyhit_shader : shaderc_glsl_default_anyhit_shader;
  if (ext == ".rcall")
    return force ? shaderc_callable_shader : shaderc_glsl_default_callable_shader;
  if (ext == ".rint")
    return force ? shaderc_intersection_shader : shaderc_glsl_default_intersection_shader;
  // Strange extension.
  return shaderc_glsl_infer_from_source;
}
#endif

shaderc_shader_kind ShaderModule::get_shader_kind() const
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

ShaderModule& ShaderModule::load(std::same_as<std::filesystem::path> auto const& filename)
{
  DoutEntering(dc::vulkan, "ShaderModule::load(" << filename << ")");

  std::ifstream ifs(filename, std::ios::ate);

  if (ifs.fail())
    THROW_ALERT("Could not open [FILENAME] file!", AIArgs("[FILENAME]", filename));

  std::streampos const file_size = ifs.tellg();
  m_glsl_source_code.resize(file_size);
  ifs.seekg(0, std::ios::beg);
  ifs.read(m_glsl_source_code.data(), file_size);

  ASSERT(ifs.gcount() == file_size);
  ASSERT(ifs.good());

  // Use constructor to set a name, or call set_name(name) before calling this function,
  // if you want to set your own name for this ShaderModule.
  if (m_name.empty())
    m_name = filename.filename();

  return *this;
}

void ShaderModule::reset()
{
  DoutEntering(dc::vulkan, "ShaderModule::reset() for shader \"" << m_name << "\".");
  m_spirv_code.clear();
  m_glsl_source_code.clear();
}

void ShaderModule::compile(ShaderCompiler const& compiler, ShaderCompilerOptions const& options)
{
  // Call load() before calling compile().
  ASSERT(!m_glsl_source_code.empty());
  // Call reset() before reusing a ShaderModule.
  ASSERT(m_spirv_code.empty());
  m_spirv_code = compiler.compile({}, *this, options);
  // Clean up.
  m_glsl_source_code.clear();
}

vk::UniqueShaderModule ShaderModule::create(utils::Badge<task::SynchronousWindow>, task::SynchronousWindow const* owning_window) const
{
  // Call compile() before calling this create().
  ASSERT(!m_spirv_code.empty());
  return owning_window->logical_device().create_shader_module(
      m_spirv_code.data(), m_spirv_code.size() * sizeof(uint32_t)
      COMMA_CWDEBUG_ONLY(vulkan::AmbifixOwner(owning_window, m_name)));
}

vk::UniqueShaderModule ShaderModule::create(task::SynchronousWindow const* owning_window, ShaderCompiler const& compiler, ShaderCompilerOptions const& options) const
{
  // Call load() before calling this create().
  ASSERT(!m_glsl_source_code.empty());
  // Call reset() before reusing a ShaderModule.
  ASSERT(m_spirv_code.empty());
  return compiler.create({}, owning_window->logical_device(), *this, options
      COMMA_CWDEBUG_ONLY(vulkan::AmbifixOwner(owning_window, m_name)));
}

#ifdef CWDEBUG
void ShaderModule::print_on(std::ostream& os) const
{
  os << '{';
  os << "name:\"" << m_name << "\", code:";
  std::istringstream glsl_code{m_glsl_source_code};
  std::string line;
  int line_number = 1;
  while (std::getline(glsl_code, line))
  {
    os << "\n" << line_number << "\t" << line;
    ++line_number;
  }
  os << "\n";
  os << "stage:" << to_string(m_stage);
  os << "}";
}
#endif

} // namespace vulkan::shaderbuilder
