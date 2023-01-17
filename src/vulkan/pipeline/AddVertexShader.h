#pragma once

#include "shader_builder/VertexShaderInputSet.h"
#include "AddShaderStage.h"
#include "shader_builder/VertexAttribute.h"
#include "shader_builder/VertexAttributeDeclarationContext.h"
#include "utils/Vector.h"
#include "utils/Badge.h"
#include "utils/log2.h"
#include <vulkan/vulkan.hpp>
#include <vector>
#include "debug.h"

namespace vulkan {
class VertexBuffers;
namespace shader_builder {
class VertexShaderInputSetBase;
} // namespace shader_builder
} // namespace vulkan

namespace vulkan::pipeline {
class CharacteristicRange;

class AddVertexShader : public virtual AddShaderStage
{
 private:
  std::vector<vk::VertexInputBindingDescription> m_vertex_input_binding_descriptions;
  std::vector<vk::VertexInputAttributeDescription> m_vertex_input_attribute_descriptions;

  //---------------------------------------------------------------------------
  // Vertex attributes.
  utils::Vector<shader_builder::VertexShaderInputSetBase*> m_vertex_shader_input_sets;          // Existing vertex shader input sets (a 'binding' slot).
  shader_builder::VertexAttributeDeclarationContext m_vertex_shader_location_context;           // Location context used for vertex attributes (VertexAttribute).
  using glsl_id_full_to_vertex_attribute_container_t = std::map<std::string, shader_builder::VertexAttribute, std::less<>>;
  mutable glsl_id_full_to_vertex_attribute_container_t m_glsl_id_full_to_vertex_attribute;      // Map VertexAttribute::m_glsl_id_full to the VertexAttribute object that contains it.
  bool m_vertex_shader_input_sets_changed{false};
  bool m_glsl_id_full_to_vertex_attribute_changed{false};

 private:
  //---------------------------------------------------------------------------
  // Vertex attributes.

  // Add shader variable (VertexAttribute) to m_glsl_id_full_to_vertex_attribute
  // (and a pointer to that to m_shader_variables), for a non-array vertex attribute.
  template<typename ContainingClass, glsl::Standard Standard, glsl::ScalarIndex ScalarIndex, int Rows, int Cols, size_t Alignment, size_t Size, size_t ArrayStride,
      int MemberIndex, size_t MaxAlignment, size_t Offset, utils::TemplateStringLiteral GlslIdStr>
  void add_vertex_attribute(VertexBufferBindingIndex binding, shader_builder::MemberLayout<ContainingClass,
      shader_builder::BasicTypeLayout<Standard, ScalarIndex, Rows, Cols, Alignment, Size, ArrayStride>,
      MemberIndex, MaxAlignment, Offset, GlslIdStr> const& member_layout);

  // Add shader variable (VertexAttribute) to m_glsl_id_full_to_vertex_attribute
  // (and a pointer to that to m_shader_variables), for a vertex attribute array.
  template<typename ContainingClass, glsl::Standard Standard, glsl::ScalarIndex ScalarIndex, int Rows, int Cols, size_t Alignment, size_t Size, size_t ArrayStride,
      int MemberIndex, size_t MaxAlignment, size_t Offset, utils::TemplateStringLiteral GlslIdStr, size_t Elements>
  void add_vertex_attribute(VertexBufferBindingIndex binding, shader_builder::MemberLayout<ContainingClass,
      shader_builder::ArrayLayout<shader_builder::BasicTypeLayout<Standard, ScalarIndex, Rows, Cols, Alignment, Size, ArrayStride>, Elements>,
      MemberIndex, MaxAlignment, Offset, GlslIdStr> const& member_layout);

  // End vertex attribute.
  //---------------------------------------------------------------------------

  void copy_vertex_input_binding_descriptions() final;
  void copy_vertex_input_attribute_descriptions() final;
  void register_AddVertexShader_with(task::PipelineFactory* pipeline_factory) const final;

 public:
  AddVertexShader() = default;

  // Called from VertexAttribute::is_used_in.
  shader_builder::VertexAttributeDeclarationContext* vertex_shader_location_context(utils::Badge<shader_builder::VertexAttribute>) override { return &m_vertex_shader_location_context; }

 protected:
  // Accessors.
  utils::Vector<shader_builder::VertexShaderInputSetBase*> const& vertex_shader_input_sets() const override { return m_vertex_shader_input_sets; }

  void add_vertex_input_bindings(VertexBuffers const& vertex_buffers);

  template<typename ENTRY>
  requires (std::same_as<typename shader_builder::ShaderVariableLayouts<ENTRY>::tag_type, glsl::per_vertex_data> ||
            std::same_as<typename shader_builder::ShaderVariableLayouts<ENTRY>::tag_type, glsl::per_instance_data>)
  void add_vertex_input_binding(shader_builder::VertexShaderInputSet<ENTRY>& vertex_shader_input_set);

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

template<typename ContainingClass, glsl::Standard Standard, glsl::ScalarIndex ScalarIndex, int Rows, int Cols, size_t Alignment, size_t Size, size_t ArrayStride,
    int MemberIndex, size_t MaxAlignment, size_t Offset, utils::TemplateStringLiteral GlslIdStr>
void AddVertexShader::add_vertex_attribute(VertexBufferBindingIndex binding, shader_builder::MemberLayout<ContainingClass,
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
  auto ibp = m_glsl_id_full_to_vertex_attribute.insert(std::pair{glsl_id_sv, vertex_attribute_tmp});
  // The m_glsl_id_full of each ENTRY must be unique. And of course, don't register the same attribute twice.
  ASSERT(ibp.second);

  // Add a pointer to the VertexAttribute that was just added to m_glsl_id_full_to_vertex_attribute to m_shader_variables.
  m_shader_variables.push_back(&ibp.first->second);
}

template<typename ContainingClass, glsl::Standard Standard, glsl::ScalarIndex ScalarIndex, int Rows, int Cols, size_t Alignment, size_t Size, size_t ArrayStride,
    int MemberIndex, size_t MaxAlignment, size_t Offset, utils::TemplateStringLiteral GlslIdStr, size_t Elements>
void AddVertexShader::add_vertex_attribute(VertexBufferBindingIndex binding, shader_builder::MemberLayout<ContainingClass,
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
  auto ibp = m_glsl_id_full_to_vertex_attribute.insert(std::pair{glsl_id_sv, vertex_attribute_tmp});
  // The m_glsl_id_full of each ENTRY must be unique. And of course, don't register the same attribute twice.
  ASSERT(ibp.second);

  // Add a pointer to the VertexAttribute that was just added to m_glsl_id_full_to_vertex_attribute to m_shader_variables.
  m_shader_variables.push_back(&ibp.first->second);
}

template<typename ENTRY>
requires (std::same_as<typename shader_builder::ShaderVariableLayouts<ENTRY>::tag_type, glsl::per_vertex_data> ||
          std::same_as<typename shader_builder::ShaderVariableLayouts<ENTRY>::tag_type, glsl::per_instance_data>)
void AddVertexShader::add_vertex_input_binding(shader_builder::VertexShaderInputSet<ENTRY>& vertex_shader_input_set)
{
  //create_vertex_buffer
  DoutEntering(dc::vulkan, "AddVertexShader::add_vertex_input_binding<" << libcwd::type_info_of<ENTRY>().demangled_name() << ">(...)");
  using namespace shader_builder;

  VertexBufferBindingIndex binding = m_vertex_shader_input_sets.iend();

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
  m_glsl_id_full_to_vertex_attribute_changed = true;

  // Keep track of all VertexShaderInputSetBase objects.
  m_vertex_shader_input_sets.push_back(&vertex_shader_input_set);
  m_vertex_shader_input_sets_changed = true;
}

} // namespace vulkan::pipeline
