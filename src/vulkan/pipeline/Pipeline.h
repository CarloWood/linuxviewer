#ifndef VULKAN_PIPELINE_PIPELINE_H
#define VULKAN_PIPELINE_PIPELINE_H

#include "shaderbuilder/ShaderInfo.h"
#include "shaderbuilder/ShaderIndex.h"
#include "shaderbuilder/SPIRVCache.h"
#include "shaderbuilder/LocationContext.h"
#include "debug/DebugSetName.h"
#include "utils/Vector.h"
#include "utils/log2.h"
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
  std::set<shaderbuilder::VertexAttribute> m_vertex_attributes;                         // All existing layouts of the above input sets (including declaration function).
  shaderbuilder::LocationContext m_vertex_shader_location_context;                      // Location context used for vertex attributes (VertexAttribute).
  std::vector<vk::PipelineShaderStageCreateInfo> m_shader_stage_create_infos;
  std::vector<vk::UniqueShaderModule> m_unique_handles;

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
  std::string_view preprocess(shaderbuilder::ShaderInfo const& shader_info, std::string& glsl_source_code_buffer,
      std::set<shaderbuilder::VertexAttribute> const* vertex_attributes = nullptr);

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

#ifndef VULKAN_PIPELINE_PIPELINE_H_definitions
#define VULKAN_PIPELINE_PIPELINE_H_definitions

namespace vulkan::pipeline {

template<typename ENTRY>
void Pipeline::add_vertex_input_binding(shaderbuilder::VertexShaderInputSet<ENTRY>& vertex_shader_input_set)
{
  DoutEntering(dc::vulkan, "vulkan::pipeline::add_vertex_input_binding<" << libcwd::type_info_of<ENTRY>().demangled_name() << ">(...)");
  using namespace shaderbuilder;

  BindingIndex binding = m_vertex_shader_input_sets.iend();

  // Insert a new VertexAttribute into m_vertex_attributes for each MemberLayout in ShaderVariableLayouts<ENTRY>::layouts.
  auto insert_vertex_attribute = [&, this]<
      typename ContainingClass, glsl::Standard Standard, glsl::ScalarIndex ScalarIndex, int Rows, int Cols, size_t Alignment, size_t Size, size_t ArrayStride,
      int MemberIndex, size_t MaxAlignment, size_t Offset, utils::TemplateStringLiteral GlslIdStr>(
          MemberLayout<
              ContainingClass, BasicTypeLayout<Standard, ScalarIndex, Rows, Cols, Alignment, Size, ArrayStride>, MemberIndex, MaxAlignment, Offset, GlslIdStr> const& member_layout)
      {
        VertexAttribute vertex_attribute{
          .m_binding = binding,
          .m_base_type = {
            .m_standard = Standard,
            .m_rows = Rows,
            .m_cols = Cols,
            .m_scalar_type = ScalarIndex,
            .m_log2_alignment = utils::log2(Alignment),
            .m_size = Size,
            .m_array_stride = ArrayStride },
          .m_glsl_id_str = GlslIdStr.chars.data(),
          .m_offset = Offset
        };
        ASSERT(false);
        auto res = m_vertex_attributes.insert(std::move(vertex_attribute));
        // All used names must be unique.
        if (!res.second)
          THROW_ALERT("Duplicated shader variable layout id \"[ID_STR]\". All used ids must be unique.", AIArgs("[ID_STR]", vertex_attribute.m_glsl_id_str));
      };

  // Use the specialization of ShaderVariableLayouts to get the layout of ENTRY
  // in the form of a tuple, of the vertex attributes, `layouts`.
  // Then for each member layout call insert_vertex_attribute.
  [&]<typename... MemberLayout>(std::tuple<MemberLayout...> const& layouts)
  {
    ([&]
    {
      {
#if 0 // Print the type MemberLayout.
        int rc;
        char const* const demangled_type = abi::__cxa_demangle(typeid(MemberLayout).name(), 0, 0, &rc);
        Dout(dc::vulkan, "We get here for type " << demangled_type);
#endif
        static constexpr int member_index = MemberLayout::member_index;
        insert_vertex_attribute(std::get<member_index>(layouts));
      }
    }(), ...);
  }(ShaderVariableLayouts<ENTRY>::layouts);

#if 0
  // Register ENTRY as vertex shader input.
  for (auto&& shader_variable_layout : shaderbuilder::ShaderVariableLayouts<ENTRY>::layouts)
  {
    auto res = m_vertex_attributes.insert(new shaderbuilder::VertexAttribute(binding, shader_variable_layout));
    // All used names must be unique.
    if (!res.second)
      THROW_ALERT("Duplicated shader variable layout id \"[ID_STR]\". All used ids must be unique.", AIArgs("[ID_STR]", shader_variable_layout.m_glsl_id_str));
  }
#endif
  // Keep track of all VertexShaderInputSetBase objects.
  m_vertex_shader_input_sets.push_back(&vertex_shader_input_set);
}

} // namespace vulkan::pipeline

#endif // VULKAN_PIPELINE_PIPELINE_H_definitions
