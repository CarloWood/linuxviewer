#ifndef VULKAN_PIPELINE_PIPELINE_H
#define VULKAN_PIPELINE_PIPELINE_H

#include "PushConstantRangeCompare.h"
#include "descriptor/SetLayout.h"
#include "descriptor/SetKeyToSetIndexHint.h"
#include "descriptor/SetKeyToShaderResourceDeclaration.h"
#include "descriptor/SetKeyPreference.h"
#include "shader_builder/ShaderInfo.h"
#include "shader_builder/ShaderIndex.h"
#include "shader_builder/SPIRVCache.h"
#include "shader_builder/VertexAttribute.h"
#include "shader_builder/VertexAttributeDeclarationContext.h"
#include "shader_builder/PushConstant.h"
#include "shader_builder/PushConstantDeclarationContext.h"
#include "shader_builder/ShaderResourceDeclaration.h"
#include "shader_builder/ShaderResourceDeclarationContext.h"
#include "shader_builder/ShaderResourceVariable.h"
#include "shader_builder/shader_resource/Base.h"
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
class PipelineFactory;
} // namespace task

namespace vulkan {

class LogicalDevice;

namespace shader_builder {

class VertexShaderInputSetBase;
template<typename ENTRY> class VertexShaderInputSet;

struct VertexAttribute;

} // namespace shader_builder

namespace shader_builder::shader_resource {
class Texture;
class UniformBufferBase;
} // namespace shader_builder::shader_resource

namespace pipeline {
class CharacteristicRange;
using CharacteristicRangeIndex = utils::VectorIndex<CharacteristicRange>;

class ShaderInputData
{
 public:
  using sorted_descriptor_set_layouts_container_t = std::vector<descriptor::SetLayout>;

 private:
  //---------------------------------------------------------------------------
  // Vertex attributes.
  utils::Vector<shader_builder::VertexShaderInputSetBase*> m_vertex_shader_input_sets;          // Existing vertex shader input sets (a 'binding' slot).
  shader_builder::VertexAttributeDeclarationContext m_vertex_shader_location_context;           // Location context used for vertex attributes (VertexAttribute).
  using glsl_id_full_to_vertex_attribute_container_t = std::map<std::string, shader_builder::VertexAttribute, std::less<>>;
  mutable glsl_id_full_to_vertex_attribute_container_t m_glsl_id_full_to_vertex_attribute;      // Map VertexAttribute::m_glsl_id_full to the VertexAttribute
  //---------------------------------------------------------------------------                 // object that contains it.

  //---------------------------------------------------------------------------
  // Push constants.
  using glsl_id_full_to_push_constant_declaration_context_container_t = std::map<std::string, std::unique_ptr<shader_builder::PushConstantDeclarationContext>>;
  glsl_id_full_to_push_constant_declaration_context_container_t m_glsl_id_full_to_push_constant_declaration_context;    // Map the prefix of VertexAttribute::m_glsl_id_full to its
                                                                                                                        // DeclarationContext object.
  using glsl_id_full_to_push_constant_container_t = std::map<std::string, shader_builder::PushConstant, std::less<>>;
  glsl_id_full_to_push_constant_container_t m_glsl_id_full_to_push_constant;                    // Map PushConstant::m_glsl_id_full to the PushConstant object that contains it.

  std::set<vk::PushConstantRange, PushConstantRangeCompare> m_push_constant_ranges;
  //---------------------------------------------------------------------------

  //---------------------------------------------------------------------------
  // Shader resources.
  using set_index_hint_to_shader_resource_declaration_context_container_t = std::map<descriptor::SetIndexHint, shader_builder::ShaderResourceDeclarationContext>;
  set_index_hint_to_shader_resource_declaration_context_container_t m_set_index_hint_to_shader_resource_declaration_context;
  using glsl_id_to_shader_resource_container_t = std::map<std::string, shader_builder::ShaderResourceDeclaration, std::less<>>;
  glsl_id_to_shader_resource_container_t m_glsl_id_to_shader_resource;
  sorted_descriptor_set_layouts_container_t m_sorted_descriptor_set_layouts;                               // A Vector of SetLayout object, containing vk::DescriptorSetLayout handles and the
                                                                                                // vk::DescriptorSetLayoutBinding objects, stored in a sorted vector, that they were
                                                                                                // created from.
  descriptor::SetKeyToSetIndexHint m_shader_resource_set_key_to_set_index_hint;                 // Maps descriptor::SetKey's to identifier::SetIndexHint's.
  using declaration_contexts_container_t = std::set<shader_builder::DeclarationContext*>;
  using per_stage_declaration_contexts_container_t = std::map<vk::ShaderStageFlagBits, declaration_contexts_container_t>;
  per_stage_declaration_contexts_container_t m_per_stage_declaration_contexts;
  descriptor::SetKeyToShaderResourceDeclaration m_shader_resource_set_key_to_shader_resource_declaration;       // Maps descriptor::SetKey's to shader resource declaration object used for the current pipeline.

  // List of shader resources that were added by the owning CharacteristicRange
  // from the add_* functions like add_uniform_buffer.
  std::vector<shader_builder::shader_resource::Base const*> m_required_shader_resources_list;

  // Descriptor set handles, sorted by descriptor set index, required for the next pipeline.
  utils::Vector<vk::DescriptorSet, descriptor::SetIndex> m_vhv_descriptor_sets;

  //---------------------------------------------------------------------------

  std::vector<shader_builder::ShaderVariable const*> m_shader_variables;                        // A list of all ShaderVariable's (elements of m_glsl_id_full_to_vertex_attribute,
                                                                                                // m_glsl_id_full_to_push_constant, m_glsl_id_to_shader_resource, ...).
  std::vector<vk::PipelineShaderStageCreateInfo> m_shader_stage_create_infos;
  std::vector<vk::UniqueShaderModule> m_shader_modules;

 private:
  //---------------------------------------------------------------------------
  // Vertex attributes.

  // Add shader variable (VertexAttribute) to m_glsl_id_full_to_vertex_attribute
  // (and a pointer to that to m_shader_variables), for a non-array vertex attribute.
  template<typename ContainingClass, glsl::Standard Standard, glsl::ScalarIndex ScalarIndex, int Rows, int Cols, size_t Alignment, size_t Size, size_t ArrayStride,
      int MemberIndex, size_t MaxAlignment, size_t Offset, utils::TemplateStringLiteral GlslIdStr>
  void add_vertex_attribute(shader_builder::BindingIndex binding, shader_builder::MemberLayout<ContainingClass,
      shader_builder::BasicTypeLayout<Standard, ScalarIndex, Rows, Cols, Alignment, Size, ArrayStride>,
      MemberIndex, MaxAlignment, Offset, GlslIdStr> const& member_layout);

  // Add shader variable (VertexAttribute) to m_glsl_id_full_to_vertex_attribute
  // (and a pointer to that to m_shader_variables), for a vertex attribute array.
  template<typename ContainingClass, glsl::Standard Standard, glsl::ScalarIndex ScalarIndex, int Rows, int Cols, size_t Alignment, size_t Size, size_t ArrayStride,
      int MemberIndex, size_t MaxAlignment, size_t Offset, utils::TemplateStringLiteral GlslIdStr, size_t Elements>
  void add_vertex_attribute(shader_builder::BindingIndex binding, shader_builder::MemberLayout<ContainingClass,
      shader_builder::ArrayLayout<shader_builder::BasicTypeLayout<Standard, ScalarIndex, Rows, Cols, Alignment, Size, ArrayStride>, Elements>,
      MemberIndex, MaxAlignment, Offset, GlslIdStr> const& member_layout);

  // End vertex attribute.
  //---------------------------------------------------------------------------

  //---------------------------------------------------------------------------
  // Push constants.

  // Add shader variable (PushConstant) to m_glsl_id_full_to_push_constant (and a pointer to that to m_shader_variables),
  // and a declaration context for its prefix) if that doesn't already exist, for a non-array push constant.
  template<typename ContainingClass, glsl::Standard Standard, glsl::ScalarIndex ScalarIndex, int Rows, int Cols, size_t Alignment, size_t Size, size_t ArrayStride,
      int MemberIndex, size_t MaxAlignment, size_t Offset, utils::TemplateStringLiteral GlslIdStr>
  void add_push_constant_member(shader_builder::MemberLayout<ContainingClass,
      shader_builder::BasicTypeLayout<Standard, ScalarIndex, Rows, Cols, Alignment, Size, ArrayStride>,
      MemberIndex, MaxAlignment, Offset, GlslIdStr> const& member_layout);

  // Add shader variable (PushConstant) to m_glsl_id_full_to_push_constant (and a pointer to that to m_shader_variables),
  // and a declaration context for its prefix) if that doesn't already exist, for a push constant array.
  template<typename ContainingClass, glsl::Standard Standard, glsl::ScalarIndex ScalarIndex, int Rows, int Cols, size_t Alignment, size_t Size, size_t ArrayStride,
      int MemberIndex, size_t MaxAlignment, size_t Offset, utils::TemplateStringLiteral GlslIdStr, size_t Elements>
  void add_push_constant_member(shader_builder::MemberLayout<ContainingClass,
      shader_builder::ArrayLayout<shader_builder::BasicTypeLayout<Standard, ScalarIndex, Rows, Cols, Alignment, Size, ArrayStride>, Elements>,
      MemberIndex, MaxAlignment, Offset, GlslIdStr> const& member_layout);

  // End push constants.
  //---------------------------------------------------------------------------

 public:
  template<typename ENTRY>
  requires (std::same_as<typename shader_builder::ShaderVariableLayouts<ENTRY>::tag_type, glsl::per_vertex_data> ||
            std::same_as<typename shader_builder::ShaderVariableLayouts<ENTRY>::tag_type, glsl::per_instance_data>)
  void add_vertex_input_binding(shader_builder::VertexShaderInputSet<ENTRY>& vertex_shader_input_set);

  template<typename ENTRY>
  requires (std::same_as<typename shader_builder::ShaderVariableLayouts<ENTRY>::tag_type, glsl::push_constant_std430>)
  void add_push_constant();

  void add_texture(shader_builder::shader_resource::Texture const& texture,
      std::vector<descriptor::SetKeyPreference> const& preferred_descriptor_sets = {},
      std::vector<descriptor::SetKeyPreference> const& undesirable_descriptor_sets = {});

  void add_uniform_buffer(shader_builder::shader_resource::UniformBufferBase const& uniform_buffer,
      std::vector<descriptor::SetKeyPreference> const& preferred_descriptor_sets = {},
      std::vector<descriptor::SetKeyPreference> const& undesirable_descriptor_sets = {});

  void request_shader_resource_creation(shader_builder::shader_resource::Base const* shader_resource);
  bool handle_shader_resource_creation_requests(task::PipelineFactory* pipeline_factory, task::SynchronousWindow const* owning_window, descriptor::SetBindingMap const& set_binding_map, ShaderInputData::sorted_descriptor_set_layouts_container_t* sorted_descriptor_set_layouts);

  void build_shader(task::SynchronousWindow const* owning_window,
      shader_builder::ShaderIndex const& shader_index, shader_builder::ShaderCompiler const& compiler,
      shader_builder::SPIRVCache& spirv_cache, descriptor::SetBindingMap const& set_binding_map
      COMMA_CWDEBUG_ONLY(AmbifixOwner const& ambifix));

  void build_shader(task::SynchronousWindow const* owning_window,
      shader_builder::ShaderIndex const& shader_index, shader_builder::ShaderCompiler const& compiler,
      descriptor::SetBindingMap const& set_binding_map
      COMMA_CWDEBUG_ONLY(AmbifixOwner const& ambifix))
  {
    shader_builder::SPIRVCache tmp_spirv_cache;
    build_shader(owning_window, shader_index, compiler, tmp_spirv_cache, set_binding_map COMMA_CWDEBUG_ONLY(ambifix));
  }

  // Create glsl code from template source code.
  //
  // glsl_source_code_buffer is only used when the code from shader_info needs preprocessing,
  // otherwise this function returns a string_view directly into the shader_info's source code.
  //
  // Hence, both shader_info and the string passed as glsl_source_code_buffer need to have a life time beyond the call to compile.
  void preprocess1(shader_builder::ShaderInfo const& shader_info);
  std::string_view preprocess2(shader_builder::ShaderInfo const& shader_info, std::string& glsl_source_code_buffer, descriptor::SetBindingMap const& set_binding_map) const;

  void push_back_descriptor_set_layout_binding(descriptor::SetIndexHint set_index_hint, vk::DescriptorSetLayoutBinding const& descriptor_set_layout_binding)
  {
    DoutEntering(dc::vulkan, "push_back_descriptor_set_layout_binding(" << set_index_hint << ", " << descriptor_set_layout_binding << ") [" << this << "]");
    Dout(dc::vulkan, "Adding " << descriptor_set_layout_binding << " to m_sorted_descriptor_set_layouts[" << set_index_hint << "].m_sorted_bindings:");
    // Find the SetLayout corresponding to the set_index_hint, if any.
    auto set_layout = std::find_if(m_sorted_descriptor_set_layouts.begin(), m_sorted_descriptor_set_layouts.end(), descriptor::CompareHint{set_index_hint});
    if (set_layout  == m_sorted_descriptor_set_layouts.end())
    {
      m_sorted_descriptor_set_layouts.emplace_back(set_index_hint);
      set_layout = m_sorted_descriptor_set_layouts.end() - 1;
    }
    set_layout->insert_descriptor_set_layout_binding(descriptor_set_layout_binding);
    // set_layout is an element of m_sorted_descriptor_set_layouts and it was just changed.
    // We need to re-sort m_sorted_descriptor_set_layouts to keep it sorted.
    std::sort(m_sorted_descriptor_set_layouts.begin(), m_sorted_descriptor_set_layouts.end(), descriptor::SetLayoutCompare{});
  }

  // Called from PushConstantDeclarationContext::glsl_id_full_is_used_in.
  void insert(vk::PushConstantRange const& push_constant_range)
  {
    auto range = m_push_constant_ranges.equal_range(push_constant_range);
    m_push_constant_ranges.erase(range.first, range.second);
    m_push_constant_ranges.insert(push_constant_range);
  }

  // Access what calls to the above insert constructed.
  std::vector<vk::PushConstantRange> push_constant_ranges() const
  {
    return { m_push_constant_ranges.begin(), m_push_constant_ranges.end() };
  }

  void realize_descriptor_set_layouts(LogicalDevice const* logical_device)
  {
    DoutEntering(dc::shaderresource|dc::vulkan, "ShaderInputData::realize_descriptor_set_layouts(" << logical_device << ") [" << this << "]");
#ifdef CWDEBUG
    Dout(dc::debug, "m_sorted_descriptor_set_layouts =");
    for (auto& descriptor_set_layout : m_sorted_descriptor_set_layouts)
      Dout(dc::debug, "    " << descriptor_set_layout);
#endif
    for (auto& descriptor_set_layout : m_sorted_descriptor_set_layouts)
      descriptor_set_layout.realize_handle(logical_device);
    Dout(dc::shaderresource, "Leaving ShaderInputData::realize_descriptor_set_layouts");
  }

  utils::Vector<vk::DescriptorSetLayout, descriptor::SetIndex> get_vhv_descriptor_set_layouts(descriptor::SetBindingMap const& set_binding_map) const
  {
    DoutEntering(dc::vulkan, "ShaderInputData::get_vhv_descriptor_set_layouts() [" << this << "]");
    // This function is called after realize_descriptor_set_layouts which means that the `binding` values
    // in the elements of SetLayout::m_sorted_bindings, of each SetLayout in m_sorted_descriptor_set_layouts,
    // is already finalized.
    Dout(dc::vulkan, "m_sorted_descriptor_set_layouts = " << m_sorted_descriptor_set_layouts);
    utils::Vector<vk::DescriptorSetLayout, descriptor::SetIndex> vhv_descriptor_set_layouts(m_sorted_descriptor_set_layouts.size());
    for (auto& descriptor_set_layout : m_sorted_descriptor_set_layouts)
    {
      descriptor::SetIndex set_index = set_binding_map.convert(descriptor_set_layout.set_index_hint());
      // This code assumes that m_sorted_descriptor_set_layouts contains contiguous range [0, 1, 2, ..., size-1] of
      // set index hint values - one for each.
      ASSERT(set_index < vhv_descriptor_set_layouts.iend());
      vhv_descriptor_set_layouts[set_index] = descriptor_set_layout.handle();
    }
    return vhv_descriptor_set_layouts;
  }

  //---------------------------------------------------------------------------
  // Accessors.

  // Used by VertexAttribute::is_used_in to access the VertexAttributeDeclarationContext.
  shader_builder::VertexAttributeDeclarationContext& vertex_shader_location_context(utils::Badge<shader_builder::VertexAttribute>) { return m_vertex_shader_location_context; }
  auto const& vertex_shader_input_sets() const { return m_vertex_shader_input_sets; }

  // Used by PushConstant::is_used_in to look up the declaration context.
  glsl_id_full_to_push_constant_declaration_context_container_t const& glsl_id_full_to_push_constant_declaration_context(utils::Badge<shader_builder::PushConstant>) const { return m_glsl_id_full_to_push_constant_declaration_context; }
  glsl_id_full_to_push_constant_container_t const& glsl_id_full_to_push_constant() const { return m_glsl_id_full_to_push_constant; }

  // Used by ShaderResourceVariable::is_used_in to look up the declaration context.
  set_index_hint_to_shader_resource_declaration_context_container_t& set_index_hint_to_shader_resource_declaration_context(utils::Badge<shader_builder::ShaderResourceVariable>) { return m_set_index_hint_to_shader_resource_declaration_context; }
//  glsl_id_to_shader_resource_container_t const& glsl_id_to_shader_resource() const { return m_glsl_id_to_shader_resource; }

  // Returns information on what was added with add_vertex_input_binding.
  std::vector<vk::VertexInputBindingDescription> vertex_binding_descriptions() const;

  // Returns information on what was added with add_vertex_input_binding.
  std::vector<vk::VertexInputAttributeDescription> vertex_input_attribute_descriptions() const;

  // Returns information on what was added with build_shader.
  std::vector<vk::PipelineShaderStageCreateInfo> const& shader_stage_create_infos() const { return m_shader_stage_create_infos; }
  sorted_descriptor_set_layouts_container_t const& sorted_descriptor_set_layouts() const { return m_sorted_descriptor_set_layouts; }
  sorted_descriptor_set_layouts_container_t& sorted_descriptor_set_layouts() { return m_sorted_descriptor_set_layouts; }

  // Returns the SetIndexHint that was assigned to this key (usually by shader_resource::add_*).
  descriptor::SetIndexHint get_set_index_hint(descriptor::SetKey set_key) const
  {
    return m_shader_resource_set_key_to_set_index_hint.get_set_index_hint(set_key);
  }

  shader_builder::ShaderResourceDeclaration const* get_declaration(descriptor::SetKey set_key) const
  {
    return m_shader_resource_set_key_to_shader_resource_declaration.get_shader_resource_declaration(set_key);
  }

  per_stage_declaration_contexts_container_t const& per_stage_declaration_contexts() const { return m_per_stage_declaration_contexts; }

  utils::Vector<vk::DescriptorSet, descriptor::SetIndex> const& vhv_descriptor_sets() const { return m_vhv_descriptor_sets; }
};

} // namespace pipeline
} // namespace vulkan

#endif // VULKAN_PIPELINE_PIPELINE_H

#ifndef VULKAN_SHADERBUILDER_VERTEX_SHADER_INPUT_SET_H
#include "shader_builder/VertexShaderInputSet.h"
#endif
#ifndef VULKAN_SHADERBUILDER_VERTEX_ATTRIBUTE_ENTRY_H
#include "shader_builder/VertexAttribute.h"
#endif
#ifndef VULKAN_APPLICATION_H
//#include "Application.h"
#endif

#ifndef VULKAN_PIPELINE_PIPELINE_H_definitions
#define VULKAN_PIPELINE_PIPELINE_H_definitions

namespace vulkan::pipeline {

template<typename ContainingClass, glsl::Standard Standard, glsl::ScalarIndex ScalarIndex, int Rows, int Cols, size_t Alignment, size_t Size, size_t ArrayStride,
    int MemberIndex, size_t MaxAlignment, size_t Offset, utils::TemplateStringLiteral GlslIdStr>
void ShaderInputData::add_vertex_attribute(shader_builder::BindingIndex binding, shader_builder::MemberLayout<ContainingClass,
    shader_builder::BasicTypeLayout<Standard, ScalarIndex, Rows, Cols, Alignment, Size, ArrayStride>,
    MemberIndex, MaxAlignment, Offset, GlslIdStr> const& member_layout)
{
  std::string_view glsl_id_sv = static_cast<std::string_view>(member_layout.glsl_id_full);
  // These strings are made with std::to_array("stringliteral"), which includes the terminating zero,
  // but the trailing '\0' was already removed by the conversion to string_view.
  ASSERT(!glsl_id_sv.empty() && glsl_id_sv.back() != '\0');
  shader_builder::VertexAttribute vertex_attribute_tmp(
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
  auto res1 = m_glsl_id_full_to_vertex_attribute.insert(std::pair{glsl_id_sv, vertex_attribute_tmp});
  // The m_glsl_id_full of each ENTRY must be unique. And of course, don't register the same attribute twice.
  ASSERT(res1.second);

  // Add a pointer to the VertexAttribute that was just added to m_glsl_id_full_to_vertex_attribute to m_shader_variables.
  m_shader_variables.push_back(&res1.first->second);
}

template<typename ContainingClass, glsl::Standard Standard, glsl::ScalarIndex ScalarIndex, int Rows, int Cols, size_t Alignment, size_t Size, size_t ArrayStride,
    int MemberIndex, size_t MaxAlignment, size_t Offset, utils::TemplateStringLiteral GlslIdStr, size_t Elements>
void ShaderInputData::add_vertex_attribute(shader_builder::BindingIndex binding, shader_builder::MemberLayout<ContainingClass,
    shader_builder::ArrayLayout<shader_builder::BasicTypeLayout<Standard, ScalarIndex, Rows, Cols, Alignment, Size, ArrayStride>, Elements>,
    MemberIndex, MaxAlignment, Offset, GlslIdStr> const& member_layout)
{
  std::string_view glsl_id_sv = static_cast<std::string_view>(member_layout.glsl_id_full);
  // These strings are made with std::to_array("stringliteral"), which includes the terminating zero,
  // but the trailing '\0' was already removed by the conversion to string_view.
  ASSERT(!glsl_id_sv.empty() && glsl_id_sv.back() != '\0');
  shader_builder::VertexAttribute vertex_attribute_tmp(
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
  auto res1 = m_glsl_id_full_to_vertex_attribute.insert(std::pair{glsl_id_sv, vertex_attribute_tmp});
  // The m_glsl_id_full of each ENTRY must be unique. And of course, don't register the same attribute twice.
  ASSERT(res1.second);

  // Add a pointer to the VertexAttribute that was just added to m_glsl_id_full_to_vertex_attribute to m_shader_variables.
  m_shader_variables.push_back(&res1.first->second);
}

template<typename ENTRY>
requires (std::same_as<typename shader_builder::ShaderVariableLayouts<ENTRY>::tag_type, glsl::per_vertex_data> ||
          std::same_as<typename shader_builder::ShaderVariableLayouts<ENTRY>::tag_type, glsl::per_instance_data>)
void ShaderInputData::add_vertex_input_binding(shader_builder::VertexShaderInputSet<ENTRY>& vertex_shader_input_set)
{
  DoutEntering(dc::vulkan, "vulkan::pipeline::add_vertex_input_binding<" << libcwd::type_info_of<ENTRY>().demangled_name() << ">(...)");
  using namespace shader_builder;

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
void ShaderInputData::add_push_constant_member(shader_builder::MemberLayout<ContainingClass,
    shader_builder::BasicTypeLayout<Standard, ScalarIndex, Rows, Cols, Alignment, Size, ArrayStride>,
    MemberIndex, MaxAlignment, Offset, GlslIdStr> const& member_layout)
{
  std::string_view glsl_id_sv = static_cast<std::string_view>(member_layout.glsl_id_full);
  shader_builder::BasicType const basic_type = { .m_standard = Standard, .m_rows = Rows, .m_cols = Cols, .m_scalar_type = ScalarIndex,
    .m_log2_alignment = utils::log2(Alignment), .m_size = Size, .m_array_stride = ArrayStride };
  shader_builder::PushConstant push_constant(basic_type, member_layout.glsl_id_full.chars.data(), Offset);
  auto res1 = m_glsl_id_full_to_push_constant.insert(std::pair{glsl_id_sv, push_constant});
  // The m_glsl_id_full of each ENTRY must be unique. And of course, don't register the same push constant twice.
  ASSERT(res1.second);
  m_shader_variables.push_back(&res1.first->second);

  // Add a PushConstantDeclarationContext with key push_constant.prefix(), if that doesn't already exists.
  if (m_glsl_id_full_to_push_constant_declaration_context.find(push_constant.prefix()) == m_glsl_id_full_to_push_constant_declaration_context.end())
  {
    std::size_t const hash = std::hash<std::string>{}(push_constant.prefix());
    DEBUG_ONLY(auto res2 =)
      m_glsl_id_full_to_push_constant_declaration_context.try_emplace(push_constant.prefix(), std::make_unique<shader_builder::PushConstantDeclarationContext>(push_constant.prefix(), hash));
    // We just used find() and it wasn't there?!
    ASSERT(res2.second);
  }
}

template<typename ContainingClass, glsl::Standard Standard, glsl::ScalarIndex ScalarIndex, int Rows, int Cols, size_t Alignment, size_t Size, size_t ArrayStride,
    int MemberIndex, size_t MaxAlignment, size_t Offset, utils::TemplateStringLiteral GlslIdStr, size_t Elements>
void ShaderInputData::add_push_constant_member(shader_builder::MemberLayout<ContainingClass,
    shader_builder::ArrayLayout<shader_builder::BasicTypeLayout<Standard, ScalarIndex, Rows, Cols, Alignment, Size, ArrayStride>, Elements>,
    MemberIndex, MaxAlignment, Offset, GlslIdStr> const& member_layout)
{
  std::string_view glsl_id_sv = static_cast<std::string_view>(member_layout.glsl_id_full);
  shader_builder::BasicType const basic_type = { .m_standard = Standard, .m_rows = Rows, .m_cols = Cols, .m_scalar_type = ScalarIndex,
    .m_log2_alignment = utils::log2(Alignment), .m_size = Size, .m_array_stride = ArrayStride };
  shader_builder::PushConstant push_constant(basic_type, member_layout.glsl_id_full.chars.data(), Offset, Elements);
  auto res1 = m_glsl_id_full_to_push_constant.insert(std::pair{glsl_id_sv, push_constant});
  // The m_glsl_id_full of each ENTRY must be unique. And of course, don't register the same push constant twice.
  ASSERT(res1.second);
  m_shader_variables.push_back(&res1.first->second);

  // Add a PushConstantDeclarationContext with key push_constant.prefix(), if that doesn't already exists.
  if (m_glsl_id_full_to_push_constant_declaration_context.find(push_constant.prefix()) == m_glsl_id_full_to_push_constant_declaration_context.end())
  {
    std::size_t const hash = std::hash<std::string>{}(push_constant.prefix());
    DEBUG_ONLY(auto res2 =)
      m_glsl_id_full_to_push_constant_declaration_context.try_emplace(push_constant.prefix(), std::make_unique<shader_builder::DeclarationContext>(push_constant.prefix(), hash));
    // We just used find() and it wasn't there?!
    ASSERT(res2.second);
  }
}

template<typename ENTRY>
requires (std::same_as<typename shader_builder::ShaderVariableLayouts<ENTRY>::tag_type, glsl::push_constant_std430>)
void ShaderInputData::add_push_constant()
{
  DoutEntering(dc::vulkan, "ShaderInputData::add_push_constant<" << libcwd::type_info_of<ENTRY>().demangled_name() << ">(...)");
  using namespace shader_builder;

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
