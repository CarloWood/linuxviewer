#ifndef VULKAN_PIPELINE_PIPELINE_H
#define VULKAN_PIPELINE_PIPELINE_H

#include "PushConstantRangeCompare.h"
#include "descriptor/SetLayout.h"
#include "descriptor/SetKeyToSetIndex.h"
#include "descriptor/SetKeyPreference.h"
#include "shaderbuilder/ShaderInfo.h"
#include "shaderbuilder/ShaderIndex.h"
#include "shaderbuilder/SPIRVCache.h"
#include "shaderbuilder/VertexAttribute.h"
#include "shaderbuilder/VertexAttributeDeclarationContext.h"
#include "shaderbuilder/PushConstant.h"
#include "shaderbuilder/PushConstantDeclarationContext.h"
#include "shaderbuilder/ShaderResource.h"
#include "shaderbuilder/ShaderResourceDeclarationContext.h"
#include "utils/Vector.h"
#include "utils/log2.h"
#include "utils/TemplateStringLiteral.h"
#include "utils/Badge.h"
#include <vector>
#include <set>
#include <tuple>
#include <memory>
#include <cxxabi.h>
#ifdef CWDEBUG
#include "debug/DebugSetName.h"
#endif

// Forward declarations.

namespace task {
class SynchronousWindow;
} // namespace task

namespace vulkan {

class LogicalDevice;

namespace shaderbuilder {

class VertexShaderInputSetBase;
template<typename ENTRY> class VertexShaderInputSet;

struct VertexAttribute;

} // namespace shaderbuilder

namespace shader_resource {
class Texture;
class UniformBuffer;
} // namespace shader_resource

namespace pipeline {

class ShaderInputData
{
 private:
  //---------------------------------------------------------------------------
  // Vertex attributes.
  utils::Vector<shaderbuilder::VertexShaderInputSetBase*> m_vertex_shader_input_sets;           // Existing vertex shader input sets (a 'binding' slot).
  shaderbuilder::VertexAttributeDeclarationContext m_vertex_shader_location_context;            // Location context used for vertex attributes (VertexAttribute).
  using glsl_id_str_to_vertex_attribute_container_t = std::map<std::string, vulkan::shaderbuilder::VertexAttribute, std::less<>>;
  mutable glsl_id_str_to_vertex_attribute_container_t m_glsl_id_str_to_vertex_attribute;        // Map VertexAttribute::m_glsl_id_str to the VertexAttribute
  //---------------------------------------------------------------------------                 // object that contains it.

  //---------------------------------------------------------------------------
  // Push constants.
  using glsl_id_str_to_push_constant_declaration_context_container_t = std::map<std::string, std::unique_ptr<shaderbuilder::PushConstantDeclarationContext>>;
  glsl_id_str_to_push_constant_declaration_context_container_t m_glsl_id_str_to_push_constant_declaration_context;          // Map the prefix of VertexAttribute::m_glsl_id_str to its DeclarationContext object.
  using glsl_id_str_to_push_constant_container_t = std::map<std::string, vulkan::shaderbuilder::PushConstant, std::less<>>;
  glsl_id_str_to_push_constant_container_t m_glsl_id_str_to_push_constant;                      // Map PushConstant::m_glsl_id_str to the PushConstant object that contains it.

  std::set<vk::PushConstantRange, PushConstantRangeCompare> m_push_constant_ranges;
  //---------------------------------------------------------------------------

  //---------------------------------------------------------------------------
  // Shader resources.
  shaderbuilder::ShaderResourceDeclarationContext m_shader_resource_declaration_context;// Declaration context used for shader resources (ShaderResource).
  using glsl_id_str_to_shader_resource_container_t = std::map<std::string, vulkan::shaderbuilder::ShaderResource, std::less<>>;
  glsl_id_str_to_shader_resource_container_t m_glsl_id_str_to_shader_resource;
  utils::Vector<descriptor::SetLayout, descriptor::SetIndex> m_sorted_descriptor_set_layouts;
  descriptor::SetKeyToSetIndex m_shader_resource_set_key_to_set_index; // Maps descriptor::SetKey's to identifier::SetIndex's.

  //---------------------------------------------------------------------------

  std::vector<shaderbuilder::ShaderVariable const*> m_shader_variables;         // A list of all ShaderVariable's (elements of m_vertex_attributes, m_glsl_id_str_to_push_constant, m_glsl_id_str_to_shader_resource, ...).
  std::vector<vk::PipelineShaderStageCreateInfo> m_shader_stage_create_infos;
  std::vector<vk::UniqueShaderModule> m_shader_modules;

 private:
  //---------------------------------------------------------------------------
  // Vertex attributes.

  // Add shader variable (VertexAttribute) to m_glsl_id_str_to_vertex_attribute
  // (and a pointer to that to m_shader_variables), for a non-array vertex attribute.
  template<typename ContainingClass, glsl::Standard Standard, glsl::ScalarIndex ScalarIndex, int Rows, int Cols, size_t Alignment, size_t Size, size_t ArrayStride,
      int MemberIndex, size_t MaxAlignment, size_t Offset, utils::TemplateStringLiteral GlslIdStr>
  void add_vertex_attribute(shaderbuilder::BindingIndex binding, shaderbuilder::MemberLayout<ContainingClass,
      shaderbuilder::BasicTypeLayout<Standard, ScalarIndex, Rows, Cols, Alignment, Size, ArrayStride>,
      MemberIndex, MaxAlignment, Offset, GlslIdStr> const& member_layout);

  // Add shader variable (VertexAttribute) to m_glsl_id_str_to_vertex_attribute
  // (and a pointer to that to m_shader_variables), for a vertex attribute array.
  template<typename ContainingClass, glsl::Standard Standard, glsl::ScalarIndex ScalarIndex, int Rows, int Cols, size_t Alignment, size_t Size, size_t ArrayStride,
      int MemberIndex, size_t MaxAlignment, size_t Offset, utils::TemplateStringLiteral GlslIdStr, size_t Elements>
  void add_vertex_attribute(shaderbuilder::BindingIndex binding, shaderbuilder::MemberLayout<ContainingClass,
      shaderbuilder::ArrayLayout<shaderbuilder::BasicTypeLayout<Standard, ScalarIndex, Rows, Cols, Alignment, Size, ArrayStride>, Elements>,
      MemberIndex, MaxAlignment, Offset, GlslIdStr> const& member_layout);

  // End vertex attribute.
  //---------------------------------------------------------------------------

  //---------------------------------------------------------------------------
  // Push constants.

  // Add shader variable (PushConstant) to m_glsl_id_str_to_push_constant (and a pointer to that to m_shader_variables),
  // and a declaration context for its prefix) if that doesn't already exist, for a non-array push constant.
  template<typename ContainingClass, glsl::Standard Standard, glsl::ScalarIndex ScalarIndex, int Rows, int Cols, size_t Alignment, size_t Size, size_t ArrayStride,
      int MemberIndex, size_t MaxAlignment, size_t Offset, utils::TemplateStringLiteral GlslIdStr>
  void add_push_constant_member(shaderbuilder::MemberLayout<ContainingClass,
      shaderbuilder::BasicTypeLayout<Standard, ScalarIndex, Rows, Cols, Alignment, Size, ArrayStride>,
      MemberIndex, MaxAlignment, Offset, GlslIdStr> const& member_layout);

  // Add shader variable (PushConstant) to m_glsl_id_str_to_push_constant (and a pointer to that to m_shader_variables),
  // and a declaration context for its prefix) if that doesn't already exist, for a push constant array.
  template<typename ContainingClass, glsl::Standard Standard, glsl::ScalarIndex ScalarIndex, int Rows, int Cols, size_t Alignment, size_t Size, size_t ArrayStride,
      int MemberIndex, size_t MaxAlignment, size_t Offset, utils::TemplateStringLiteral GlslIdStr, size_t Elements>
  void add_push_constant_member(shaderbuilder::MemberLayout<ContainingClass,
      shaderbuilder::ArrayLayout<shaderbuilder::BasicTypeLayout<Standard, ScalarIndex, Rows, Cols, Alignment, Size, ArrayStride>, Elements>,
      MemberIndex, MaxAlignment, Offset, GlslIdStr> const& member_layout);

  // End push constants.
  //---------------------------------------------------------------------------

 public:
  ShaderInputData() : m_shader_resource_declaration_context(this) { }

  template<typename ENTRY>
  requires (std::same_as<typename shaderbuilder::ShaderVariableLayouts<ENTRY>::tag_type, glsl::per_vertex_data> ||
            std::same_as<typename shaderbuilder::ShaderVariableLayouts<ENTRY>::tag_type, glsl::per_instance_data>)
  void add_vertex_input_binding(shaderbuilder::VertexShaderInputSet<ENTRY>& vertex_shader_input_set);

  template<typename ENTRY>
  requires (std::same_as<typename shaderbuilder::ShaderVariableLayouts<ENTRY>::tag_type, glsl::push_constant_std430>)
  void add_push_constant();

  void add_texture(shader_resource::Texture const& texture,
      std::vector<descriptor::SetKeyPreference> const& preferred_descriptor_sets = {},
      std::vector<descriptor::SetKeyPreference> const& undesirable_descriptor_sets = {});

  void add_uniform_buffer(shader_resource::UniformBuffer const& uniform_buffer,
      std::vector<descriptor::SetKeyPreference> const& preferred_descriptor_sets = {},
      std::vector<descriptor::SetKeyPreference> const& undesirable_descriptor_sets = {});

  // Reserve one shader resource binding for the set with set_index.
  void reserve_binding(descriptor::SetKey set_key) { m_shader_resource_declaration_context.reserve_binding(set_key); }

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

  // Called indirectly from preprocess whenever we see a shader resource being used that uses `set_index`.
  void saw_set(descriptor::SetIndex set_index)
  {
    if (set_index >= m_sorted_descriptor_set_layouts.iend())
      m_sorted_descriptor_set_layouts.resize(set_index.get_value() + 1);
  }

  void push_back_descriptor_set_layout_binding(descriptor::SetIndex set_index, vk::DescriptorSetLayoutBinding const& descriptor_set_layout_binding)
  {
    m_sorted_descriptor_set_layouts[set_index].push_back(descriptor_set_layout_binding);
  }

  // Called from PushConstantDeclarationContext::glsl_id_str_is_used_in.
  void insert(vk::PushConstantRange const& push_constant_range)
  {
    auto range = m_push_constant_ranges.equal_range(push_constant_range);
    m_push_constant_ranges.erase(range.first, range.second);
    m_push_constant_ranges.insert(push_constant_range);
  }

  std::vector<vk::PushConstantRange> push_constant_ranges() const
  {
    return { m_push_constant_ranges.begin(), m_push_constant_ranges.end() };
  }

  std::vector<vk::DescriptorSetLayout> realize_descriptor_set_layouts(LogicalDevice const* logical_device)
  {
    DoutEntering(dc::vulkan, "ShaderInputData::realize_descriptor_set_layouts(" << logical_device << ")");
    Dout(dc::vulkan, "m_sorted_descriptor_set_layouts = " << m_sorted_descriptor_set_layouts);
    std::vector<vk::DescriptorSetLayout> vhv_descriptor_set_layouts;
    for (auto& descriptor_set_layout : m_sorted_descriptor_set_layouts)
    {
      descriptor_set_layout.realize_handle(logical_device);
      vhv_descriptor_set_layouts.push_back(descriptor_set_layout.handle());
    }
    return vhv_descriptor_set_layouts;
  }

  //---------------------------------------------------------------------------
  // Accessors.

  // Used by VertexAttribute::is_used_in to access the VertexAttributeDeclarationContext.
  shaderbuilder::VertexAttributeDeclarationContext& vertex_shader_location_context(utils::Badge<shaderbuilder::VertexAttribute>) { return m_vertex_shader_location_context; }
  auto const& vertex_shader_input_sets() const { return m_vertex_shader_input_sets; }

  // Used by PushConstant::is_used_in to look up the declaration context.
  glsl_id_str_to_push_constant_declaration_context_container_t const& glsl_id_str_to_push_constant_declaration_context(utils::Badge<shaderbuilder::PushConstant>) const { return m_glsl_id_str_to_push_constant_declaration_context; }
  glsl_id_str_to_push_constant_container_t const& glsl_id_str_to_push_constant() const { return m_glsl_id_str_to_push_constant; }

  // Returns information on what was added with add_vertex_input_binding.
  std::vector<vk::VertexInputBindingDescription> vertex_binding_descriptions() const;

  // Returns information on what was added with add_vertex_input_binding.
  std::vector<vk::VertexInputAttributeDescription> vertex_input_attribute_descriptions() const;

  // Returns information on what was added with build_shader.
  shaderbuilder::ShaderResourceDeclarationContext& shader_resource_context(utils::Badge<shaderbuilder::ShaderResource>) { return m_shader_resource_declaration_context; }
  std::vector<vk::PipelineShaderStageCreateInfo> const& shader_stage_create_infos() const { return m_shader_stage_create_infos; }
  utils::Vector<descriptor::SetLayout, descriptor::SetIndex> const& sorted_descriptor_set_layouts() const { return m_sorted_descriptor_set_layouts; }
};

} // namespace pipeline
} // namespace vulkan

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
void ShaderInputData::add_vertex_attribute(shaderbuilder::BindingIndex binding, shaderbuilder::MemberLayout<ContainingClass,
    shaderbuilder::BasicTypeLayout<Standard, ScalarIndex, Rows, Cols, Alignment, Size, ArrayStride>,
    MemberIndex, MaxAlignment, Offset, GlslIdStr> const& member_layout)
{
  std::string_view glsl_id_sv = static_cast<std::string_view>(member_layout.glsl_id_str);
  // These strings are made with std::to_array("stringliteral"), which includes the terminating zero,
  // but the trailing '\0' was already removed by the conversion to string_view.
  ASSERT(!glsl_id_sv.empty() && glsl_id_sv.back() != '\0');
  shaderbuilder::VertexAttribute vertex_attribute_tmp(
    { .m_standard = Standard,
      .m_rows = Rows,
      .m_cols = Cols,
      .m_scalar_type = ScalarIndex,
      .m_log2_alignment = utils::log2(Alignment),
      .m_size = Size,
      .m_array_stride = ArrayStride },
    glsl_id_sv.data(),
    Offset,
    0 /* array_size */,
    binding
  );
  Dout(dc::vulkan, "Registering \"" << glsl_id_sv << "\" with vertex attribute " << vertex_attribute_tmp);
  auto res1 = m_glsl_id_str_to_vertex_attribute.insert(std::pair{glsl_id_sv, vertex_attribute_tmp});
  // The m_glsl_id_str of each ENTRY must be unique. And of course, don't register the same attribute twice.
  ASSERT(res1.second);

  // Add a pointer to the VertexAttribute that was just added to m_glsl_id_str_to_vertex_attribute to m_shader_variables.
  m_shader_variables.push_back(&res1.first->second);
}

template<typename ContainingClass, glsl::Standard Standard, glsl::ScalarIndex ScalarIndex, int Rows, int Cols, size_t Alignment, size_t Size, size_t ArrayStride,
    int MemberIndex, size_t MaxAlignment, size_t Offset, utils::TemplateStringLiteral GlslIdStr, size_t Elements>
void ShaderInputData::add_vertex_attribute(shaderbuilder::BindingIndex binding, shaderbuilder::MemberLayout<ContainingClass,
    shaderbuilder::ArrayLayout<shaderbuilder::BasicTypeLayout<Standard, ScalarIndex, Rows, Cols, Alignment, Size, ArrayStride>, Elements>,
    MemberIndex, MaxAlignment, Offset, GlslIdStr> const& member_layout)
{
  std::string_view glsl_id_sv = static_cast<std::string_view>(member_layout.glsl_id_str);
  // These strings are made with std::to_array("stringliteral"), which includes the terminating zero,
  // but the trailing '\0' was already removed by the conversion to string_view.
  ASSERT(!glsl_id_sv.empty() && glsl_id_sv.back() != '\0');
  shaderbuilder::VertexAttribute vertex_attribute_tmp(
    { .m_standard = Standard,
      .m_rows = Rows,
      .m_cols = Cols,
      .m_scalar_type = ScalarIndex,
      .m_log2_alignment = utils::log2(Alignment),
      .m_size = Size,
      .m_array_stride = ArrayStride },
    glsl_id_sv.data(),
    Offset,
    Elements,
    binding
  );
  Dout(dc::vulkan, "Registering \"" << glsl_id_sv << "\" with vertex attribute " << vertex_attribute_tmp);
  auto res1 = m_glsl_id_str_to_vertex_attribute.insert(std::pair{glsl_id_sv, vertex_attribute_tmp});
  // The m_glsl_id_str of each ENTRY must be unique. And of course, don't register the same attribute twice.
  ASSERT(res1.second);

  // Add a pointer to the VertexAttribute that was just added to m_glsl_id_str_to_vertex_attribute to m_shader_variables.
  m_shader_variables.push_back(&res1.first->second);
}

template<typename ENTRY>
requires (std::same_as<typename shaderbuilder::ShaderVariableLayouts<ENTRY>::tag_type, glsl::per_vertex_data> ||
          std::same_as<typename shaderbuilder::ShaderVariableLayouts<ENTRY>::tag_type, glsl::per_instance_data>)
void ShaderInputData::add_vertex_input_binding(shaderbuilder::VertexShaderInputSet<ENTRY>& vertex_shader_input_set)
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
  }(typename decltype(ShaderVariableLayouts<ENTRY>::struct_layout)::members_tuple{});

  // Keep track of all VertexShaderInputSetBase objects.
  m_vertex_shader_input_sets.push_back(&vertex_shader_input_set);
}

template<typename ContainingClass, glsl::Standard Standard, glsl::ScalarIndex ScalarIndex, int Rows, int Cols, size_t Alignment, size_t Size, size_t ArrayStride,
    int MemberIndex, size_t MaxAlignment, size_t Offset, utils::TemplateStringLiteral GlslIdStr>
void ShaderInputData::add_push_constant_member(shaderbuilder::MemberLayout<ContainingClass,
    shaderbuilder::BasicTypeLayout<Standard, ScalarIndex, Rows, Cols, Alignment, Size, ArrayStride>,
    MemberIndex, MaxAlignment, Offset, GlslIdStr> const& member_layout)
{
  std::string_view glsl_id_sv = static_cast<std::string_view>(member_layout.glsl_id_str);
  shaderbuilder::BasicType const basic_type = { .m_standard = Standard, .m_rows = Rows, .m_cols = Cols, .m_scalar_type = ScalarIndex,
    .m_log2_alignment = utils::log2(Alignment), .m_size = Size, .m_array_stride = ArrayStride };
  shaderbuilder::PushConstant push_constant(basic_type, member_layout.glsl_id_str.chars.data(), Offset);
  auto res1 = m_glsl_id_str_to_push_constant.insert(std::pair{glsl_id_sv, push_constant});
  // The m_glsl_id_str of each ENTRY must be unique. And of course, don't register the same push constant twice.
  ASSERT(res1.second);
  m_shader_variables.push_back(&res1.first->second);

  // Add a PushConstantDeclarationContext with key push_constant.prefix(), if that doesn't already exists.
  if (m_glsl_id_str_to_push_constant_declaration_context.find(push_constant.prefix()) == m_glsl_id_str_to_push_constant_declaration_context.end())
  {
    std::size_t const hash = std::hash<std::string>{}(push_constant.prefix());
    DEBUG_ONLY(auto res2 =)
      m_glsl_id_str_to_push_constant_declaration_context.try_emplace(push_constant.prefix(), std::make_unique<shaderbuilder::PushConstantDeclarationContext>(push_constant.prefix(), hash));
    // We just used find() and it wasn't there?!
    ASSERT(res2.second);
  }
}

template<typename ContainingClass, glsl::Standard Standard, glsl::ScalarIndex ScalarIndex, int Rows, int Cols, size_t Alignment, size_t Size, size_t ArrayStride,
    int MemberIndex, size_t MaxAlignment, size_t Offset, utils::TemplateStringLiteral GlslIdStr, size_t Elements>
void ShaderInputData::add_push_constant_member(shaderbuilder::MemberLayout<ContainingClass,
    shaderbuilder::ArrayLayout<shaderbuilder::BasicTypeLayout<Standard, ScalarIndex, Rows, Cols, Alignment, Size, ArrayStride>, Elements>,
    MemberIndex, MaxAlignment, Offset, GlslIdStr> const& member_layout)
{
  std::string_view glsl_id_sv = static_cast<std::string_view>(member_layout.glsl_id_str);
  shaderbuilder::BasicType const basic_type = { .m_standard = Standard, .m_rows = Rows, .m_cols = Cols, .m_scalar_type = ScalarIndex,
    .m_log2_alignment = utils::log2(Alignment), .m_size = Size, .m_array_stride = ArrayStride };
  shaderbuilder::PushConstant push_constant(basic_type, member_layout.glsl_id_str.chars.data(), Offset, Elements);
  auto res1 = m_glsl_id_str_to_push_constant.insert(std::pair{glsl_id_sv, push_constant});
  // The m_glsl_id_str of each ENTRY must be unique. And of course, don't register the same push constant twice.
  ASSERT(res1.second);
  m_shader_variables.push_back(&res1.first->second);

  // Add a PushConstantDeclarationContext with key push_constant.prefix(), if that doesn't already exists.
  if (m_glsl_id_str_to_push_constant_declaration_context.find(push_constant.prefix()) == m_glsl_id_str_to_push_constant_declaration_context.end())
  {
    std::size_t const hash = std::hash<std::string>{}(push_constant.prefix());
    DEBUG_ONLY(auto res2 =)
      m_glsl_id_str_to_push_constant_declaration_context.try_emplace(push_constant.prefix(), std::make_unique<shaderbuilder::DeclarationContext>(push_constant.prefix(), hash));
    // We just used find() and it wasn't there?!
    ASSERT(res2.second);
  }
}

template<typename ENTRY>
requires (std::same_as<typename shaderbuilder::ShaderVariableLayouts<ENTRY>::tag_type, glsl::push_constant_std430>)
void ShaderInputData::add_push_constant()
{
  DoutEntering(dc::vulkan, "ShaderInputData::add_push_constant<" << libcwd::type_info_of<ENTRY>().demangled_name() << ">(...)");
  using namespace shaderbuilder;

  [&]<typename... MemberLayout>(std::tuple<MemberLayout...> const& layouts)
  {
    ([&, this]
    {
      {
#if 1 // Print the type MemberLayout.
        int rc;
        char const* const demangled_type = abi::__cxa_demangle(typeid(MemberLayout).name(), 0, 0, &rc);
        Dout(dc::vulkan, "We get here for type " << demangled_type);
#endif
        static constexpr int member_index = MemberLayout::member_index;
        add_push_constant_member(std::get<member_index>(layouts));
      }
    }(), ...);
  }(typename decltype(ShaderVariableLayouts<ENTRY>::struct_layout)::members_tuple{});
}

} // namespace vulkan::pipeline

#endif // VULKAN_PIPELINE_PIPELINE_H_definitions
