#ifndef VULKAN_PIPELINE_ADD_PUSH_CONSTANT_H
#include "AddPushConstant.h"
#endif

#ifndef VULKAN_SHADERBUILDER_VERTEX_SHADER_INPUT_SET_H
#include "shader_builder/VertexShaderInputSet.h"
#endif
#ifndef VULKAN_SHADERBUILDER_VERTEX_ATTRIBUTE_ENTRY_H
#include "shader_builder/VertexAttribute.h"
#endif
#ifndef VULKAN_PIPELINE_ADD_VERTEX_SHADER_H
#include "pipeline/AddVertexShader.h"
#endif

#ifndef VULKAN_PIPELINE_ADD_PUSH_CONSTANT_H_definitions
#define VULKAN_PIPELINE_ADD_PUSH_CONSTANT_H_definitions

namespace vulkan::pipeline {

template<typename ContainingClass, glsl::Standard Standard, glsl::ScalarIndex ScalarIndex, int Rows, int Cols, size_t Alignment, size_t Size, size_t ArrayStride,
    int MemberIndex, size_t MaxAlignment, size_t Offset, utils::TemplateStringLiteral GlslIdStr>
void AddPushConstant::add_push_constant_member(shader_builder::MemberLayout<ContainingClass,
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

  // Call AddShaderStageBridge function to add this 'shader variable'.
  add_shader_variable(&res1.first->second);

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
void AddPushConstant::add_push_constant_member(shader_builder::MemberLayout<ContainingClass,
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

  // Call AddShaderStageBridge function to add this 'shader variable'.
  add_shader_variable(&res1.first->second);

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
void AddPushConstant::add_push_constant()
{
  DoutEntering(dc::vulkan, "AddPushConstant::add_push_constant<" << libcwd::type_info_of<ENTRY>().demangled_name() << ">(...)");
  using namespace shader_builder;

  [&]<typename... MemberLayout>(std::tuple<MemberLayout...> const& layouts)
  {
    ([&, this]
    {
      {
#if 0 // Print the type MemberLayout.
        int rc;
        char const* const demangled_type = abi::__cxa_demangle(typeid(MemberLayout).name(), 0, 0, &rc);
        Dout(dc::vulkan, "Calling add_push_constant_member(" << demangled_type << ")");
#endif
        static constexpr int member_index = MemberLayout::member_index;
        add_push_constant_member(std::get<member_index>(layouts));
      }
    }(), ...);
  }(typename decltype(ShaderVariableLayouts<ENTRY>::struct_layout)::members_tuple{});
}

} // namespace vulkan::pipeline

#endif // VULKAN_PIPELINE_ADD_PUSH_CONSTANT_H_definitions
