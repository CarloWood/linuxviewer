#ifndef VULKAN_PIPELINE_PIPELINE_H
#define VULKAN_PIPELINE_PIPELINE_H

#include "shaderbuilder/ShaderInfo.h"
#include "shaderbuilder/ShaderIndex.h"
#include "shaderbuilder/SPIRVCache.h"
#include "shaderbuilder/LocationContext.h"
#include "shaderbuilder/VertexAttribute.h"
#include "debug/DebugSetName.h"
#include "utils/Vector.h"
#include "utils/log2.h"
#include "utils/TemplateStringLiteral.h"
#include <vector>
#include <set>
#include <tuple>
#include <cxxabi.h>

// Forward declarations.

namespace task {
class SynchronousWindow;
} // namespace task

namespace vulkan::shaderbuilder {

class VertexShaderInputSetBase;
template<typename ENTRY> class VertexShaderInputSet;

struct VertexAttribute;

} // namespace vulkan::shaderbuilder

namespace vulkan::pipeline {

class Pipeline
{
  utils::Vector<shaderbuilder::VertexShaderInputSetBase*> m_vertex_shader_input_sets;   // Existing vertex shader input sets (a 'binding' slot).
  std::set<shaderbuilder::VertexAttribute> m_vertex_attributes;                         // All existing vertex attributes of the above input sets (including declaration function).
  shaderbuilder::LocationContext m_vertex_shader_location_context;                      // Location context used for vertex attributes (VertexAttribute).
  std::vector<shaderbuilder::ShaderVariable const*> m_shader_variables;                 // A list of all ShaderVariable's (elements of m_vertex_attributes, ...).
  std::vector<vk::PipelineShaderStageCreateInfo> m_shader_stage_create_infos;
  std::vector<vk::UniqueShaderModule> m_unique_handles;

 private:
  template<typename ContainingClass, glsl::Standard Standard, glsl::ScalarIndex ScalarIndex, int Rows, int Cols, size_t Alignment, size_t Size, size_t ArrayStride,
      int MemberIndex, size_t MaxAlignment, size_t Offset, utils::TemplateStringLiteral GlslIdStr>
  void add_vertex_attribute(shaderbuilder::BindingIndex binding, shaderbuilder::MemberLayout<ContainingClass,
      shaderbuilder::BasicTypeLayout<Standard, ScalarIndex, Rows, Cols, Alignment, Size, ArrayStride>,
      MemberIndex, MaxAlignment, Offset, GlslIdStr> const& member_layout);

  template<typename ContainingClass, glsl::Standard Standard, glsl::ScalarIndex ScalarIndex, int Rows, int Cols, size_t Alignment, size_t Size, size_t ArrayStride,
      int MemberIndex, size_t MaxAlignment, size_t Offset, utils::TemplateStringLiteral GlslIdStr, size_t Elements>
  void add_vertex_attribute(shaderbuilder::BindingIndex binding, shaderbuilder::MemberLayout<ContainingClass,
      shaderbuilder::ArrayLayout<shaderbuilder::BasicTypeLayout<Standard, ScalarIndex, Rows, Cols, Alignment, Size, ArrayStride>, Elements>,
      MemberIndex, MaxAlignment, Offset, GlslIdStr> const& member_layout);

 public:
  template<typename ENTRY>
  void add_vertex_input_binding(shaderbuilder::VertexShaderInputSet<ENTRY>& vertex_shader_input_set);

  void build_shader(task::SynchronousWindow const* owning_window, shaderbuilder::ShaderIndex const& shader_index, shaderbuilder::ShaderCompiler const& compiler,
      shaderbuilder::SPIRVCache& spirv_cache
      COMMA_CWDEBUG_ONLY(AmbifixOwner const& ambifix));

  void build_shader(task::SynchronousWindow const* owning_window, shaderbuilder::ShaderIndex const& shader_index, shaderbuilder::ShaderCompiler const& compiler
      COMMA_CWDEBUG_ONLY(AmbifixOwner const& ambifix))
  {
    shaderbuilder::SPIRVCache tmp_spirv_cache;
    build_shader(owning_window, shader_index, compiler, tmp_spirv_cache COMMA_CWDEBUG_ONLY(ambifix));
  }

  // Create glsl code from template source code.
  //
  // glsl_source_code_buffer is only used when the code from shader_info needs preprocessing,
  // otherwise this function returns a string_view directly into the shader_info's source code.
  //
  // Hence, both shader_info and the string passed as glsl_source_code_buffer need to have a life time beyond the call to compile.
  std::string_view preprocess(shaderbuilder::ShaderInfo const& shader_info, std::string& glsl_source_code_buffer);

  // Accessors.
  auto const& vertex_shader_input_sets() const { return m_vertex_shader_input_sets; }
  auto& vertex_shader_location_context() { return m_vertex_shader_location_context; }

  // Returns information on what was added with add_vertex_input_binding.
  std::vector<vk::VertexInputBindingDescription> vertex_binding_descriptions() const;

  // Returns information on what was added with add_vertex_input_binding.
  std::vector<vk::VertexInputAttributeDescription> vertex_input_attribute_descriptions() const;

  // Returns information on what was added with build_shader.
  std::vector<vk::PipelineShaderStageCreateInfo> const& shader_stage_create_infos() const { return m_shader_stage_create_infos; }
};

} // namespace vulkan::pipeline

#endif // VULKAN_PIPELINE_PIPELINE_H

#ifndef VULKAN_SHADERBUILDER_VERTEX_SHADER_INPUT_SET_H
#include "shaderbuilder/VertexShaderInputSet.h"
#endif
#ifndef VULKAN_SHADERBUILDER_VERTEX_ATTRIBUTE_ENTRY_H
#include "shaderbuilder/VertexAttribute.h"
#endif
#ifndef VULKAN_APPLICATION_H
#include "Application.h"
#endif

#ifndef VULKAN_PIPELINE_PIPELINE_H_definitions
#define VULKAN_PIPELINE_PIPELINE_H_definitions

namespace vulkan::pipeline {

template<typename ContainingClass, glsl::Standard Standard, glsl::ScalarIndex ScalarIndex, int Rows, int Cols, size_t Alignment, size_t Size, size_t ArrayStride,
    int MemberIndex, size_t MaxAlignment, size_t Offset, utils::TemplateStringLiteral GlslIdStr>
void Pipeline::add_vertex_attribute(shaderbuilder::BindingIndex binding, shaderbuilder::MemberLayout<ContainingClass,
    shaderbuilder::BasicTypeLayout<Standard, ScalarIndex, Rows, Cols, Alignment, Size, ArrayStride>,
    MemberIndex, MaxAlignment, Offset, GlslIdStr> const& member_layout)
{
  shaderbuilder::VertexAttributeLayout const* vertex_attribute_layout = Application::instance().get_vertex_attribute_layout(GlslIdStr);
  auto res = m_vertex_attributes.emplace(vertex_attribute_layout, binding);
  // All used names must be unique.
  if (!res.second)
    THROW_ALERT("Duplicated shader variable layout id \"[ID_STR]\". All used ids must be unique.", AIArgs("[ID_STR]", vertex_attribute_layout->m_glsl_id_str));
  // Add a pointer to the VertexAttribute that was just added to m_vertex_attributes to m_shader_variables.
  m_shader_variables.push_back(&*res.first);
}

template<typename ContainingClass, glsl::Standard Standard, glsl::ScalarIndex ScalarIndex, int Rows, int Cols, size_t Alignment, size_t Size, size_t ArrayStride,
    int MemberIndex, size_t MaxAlignment, size_t Offset, utils::TemplateStringLiteral GlslIdStr, size_t Elements>
void Pipeline::add_vertex_attribute(shaderbuilder::BindingIndex binding, shaderbuilder::MemberLayout<ContainingClass,
    shaderbuilder::ArrayLayout<shaderbuilder::BasicTypeLayout<Standard, ScalarIndex, Rows, Cols, Alignment, Size, ArrayStride>, Elements>,
    MemberIndex, MaxAlignment, Offset, GlslIdStr> const& member_layout)
{
  shaderbuilder::VertexAttributeLayout const* vertex_attribute_layout = Application::instance().get_vertex_attribute_layout(GlslIdStr);
  // m_array_size should have been set during the call to Application::register_vertex_attributes.
  ASSERT(vertex_attribute_layout->m_array_size == Elements);
  auto res = m_vertex_attributes.emplace(vertex_attribute_layout, binding);
  // All used names must be unique.
  if (!res.second)
    THROW_ALERT("Duplicated shader variable layout id \"[ID_STR]\". All used ids must be unique.", AIArgs("[ID_STR]", vertex_attribute_layout->m_glsl_id_str));
  // Add a pointer to the VertexAttribute that was just added to m_vertex_attributes to m_shader_variables.
  m_shader_variables.push_back(&*res.first);
}

template<typename ENTRY>
void Pipeline::add_vertex_input_binding(shaderbuilder::VertexShaderInputSet<ENTRY>& vertex_shader_input_set)
{
  DoutEntering(dc::vulkan, "vulkan::pipeline::add_vertex_input_binding<" << libcwd::type_info_of<ENTRY>().demangled_name() << ">(...)");
  using namespace shaderbuilder;

  BindingIndex binding = m_vertex_shader_input_sets.iend();

  // Use the specialization of ShaderVariableLayouts to get the layout of ENTRY
  // in the form of a tuple, of the vertex attributes, `layouts`.
  // Then for each member layout call insert_vertex_attribute.
  [&]<typename... MemberLayout>(std::tuple<MemberLayout...> const& layouts)
  {
    ([&, this]
    {
      {
#if 0 // Print the type MemberLayout.
        int rc;
        char const* const demangled_type = abi::__cxa_demangle(typeid(MemberLayout).name(), 0, 0, &rc);
        Dout(dc::vulkan, "We get here for type " << demangled_type);
#endif
        static constexpr int member_index = MemberLayout::member_index;
        add_vertex_attribute(binding, std::get<member_index>(layouts));
      }
    }(), ...);
  }(ShaderVariableLayouts<ENTRY>::layouts);

  // Keep track of all VertexShaderInputSetBase objects.
  m_vertex_shader_input_sets.push_back(&vertex_shader_input_set);
}

} // namespace vulkan::pipeline

#endif // VULKAN_PIPELINE_PIPELINE_H_definitions
