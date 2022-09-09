#include "sys.h"
#include "SPIRVCache.h"
#include "LogicalDevice.h"
#include "SynchronousWindow.h"
#include "pipeline/ShaderInputData.h"
#include "utils/AIAlert.h"
#include "utils/at_scope_end.h"
#include <shaderc/shaderc.hpp>
#include <magic_enum.hpp>
#include <fstream>
#include <functional>
#include "debug.h"
#ifdef CWDEBUG
#include <sstream>
#endif

std::ostream& operator<<(std::ostream& os, shaderc_shader_kind kind)
{
  return os << magic_enum::enum_name(kind);
}

namespace vulkan::shader_builder {

#if 0
shaderc_shader_kind SPIRVCache::filename_to_shader_kind(std::filesystem::path filename, bool force) const
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

void SPIRVCache::reset()
{
  DoutEntering(dc::vulkan, "SPIRVCache::reset() [" << this << "]");
  m_spirv_code.clear();
}

void SPIRVCache::compile(std::string_view glsl_source_code, ShaderCompiler const& compiler, ShaderInfo const& shader_info)
{
  DoutEntering(dc::vulkan, "SPIRVCache::compile(...)");

  // Call reset() before reusing a SPIRVCache.
  ASSERT(m_spirv_code.empty());
  m_spirv_code = compiler.compile({}, shader_info, glsl_source_code);
}

vk::UniqueShaderModule SPIRVCache::create_module(utils::Badge<vulkan::pipeline::ShaderInputData>, vulkan::LogicalDevice const* logical_device
    COMMA_CWDEBUG_ONLY(vulkan::AmbifixOwner const& debug_name)) const
{
  DoutEntering(dc::vulkan, "SPIRVCache::create({}, " << logical_device << ")");

  // Call compile() before calling this create().
  ASSERT(!m_spirv_code.empty());
  return logical_device->create_shader_module(m_spirv_code.data(), m_spirv_code.size() * sizeof(uint32_t)
      COMMA_CWDEBUG_ONLY(debug_name));
}

} // namespace vulkan::shader_builder
