#pragma once
#undef LV_NEEDS_ADD_PUSH_CONSTANT_INL_H

#ifndef VULKAN_PIPELINE_ADD_PUSH_CONSTANT_H
#include "AddPushConstant.h"
#endif
#ifndef VULKAN_PIPELINE_ADD_VERTEX_SHADER_H
#include "AddVertexShader.h"
#endif

#ifndef VULKAN_SHADERBUILDER_VERTEX_SHADER_INPUT_SET_H
#include "../shader_builder/VertexShaderInputSet.h"
#endif
#ifndef VULKAN_SHADERBUILDER_VERTEX_ATTRIBUTE_ENTRY_H
#include "../shader_builder/VertexAttribute.h"
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
  shader_builder::PushConstant push_constant{typeid(ContainingClass), basic_type, member_layout.glsl_id_full.chars.data(), Offset};
  auto ibp1 = m_glsl_id_full_to_push_constant.insert(std::pair{glsl_id_sv, push_constant});
  // The m_glsl_id_full of each ENTRY must be unique. And of course, don't register the same push constant twice.
  ASSERT(ibp1.second);

  // Call AddShaderStageBridge function to add this 'shader variable'.
  add_shader_variable(&ibp1.first->second);

  // Add a PushConstantDeclarationContext with key push_constant.prefix(), if that doesn't already exists.
  if (m_glsl_id_full_to_push_constant_declaration_context.find(push_constant.prefix()) == m_glsl_id_full_to_push_constant_declaration_context.end())
  {
    std::size_t const hash = std::hash<std::string>{}(push_constant.prefix());
    DEBUG_ONLY(auto ibp2 =)
      m_glsl_id_full_to_push_constant_declaration_context.try_emplace(push_constant.prefix(), std::make_unique<shader_builder::PushConstantDeclarationContext>(push_constant.prefix(), hash));
    // We just used find() and it wasn't there?!
    ASSERT(ibp2.second);
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
  shader_builder::PushConstant push_constant{basic_type, member_layout.glsl_id_full.chars.data(), Offset, Elements};
  auto ibp1 = m_glsl_id_full_to_push_constant.insert(std::pair{glsl_id_sv, push_constant});
  // The m_glsl_id_full of each ENTRY must be unique. And of course, don't register the same push constant twice.
  ASSERT(ibp1.second);

  // Call AddShaderStageBridge function to add this 'shader variable'.
  add_shader_variable(&ibp1.first->second);

  // Add a PushConstantDeclarationContext with key push_constant.prefix(), if that doesn't already exists.
  if (m_glsl_id_full_to_push_constant_declaration_context.find(push_constant.prefix()) == m_glsl_id_full_to_push_constant_declaration_context.end())
  {
    std::size_t const hash = std::hash<std::string>{}(push_constant.prefix());
    DEBUG_ONLY(auto ibp2 =)
      m_glsl_id_full_to_push_constant_declaration_context.try_emplace(push_constant.prefix(), std::make_unique<shader_builder::PushConstantDeclarationContext>(push_constant.prefix(), hash));
    // We just used find() and it wasn't there?!
    ASSERT(ibp2.second);
  }
}

// Note: push_constant_range_out is passed as a const&, but we DO change it (in a thread-safe way)!
// The atomic PushConstantRange::m_shader_stage_flags is updated with a RMW operation during
// preprocessing of the shader(s) that use it.
template<typename ENTRY>
requires (std::same_as<typename shader_builder::ShaderVariableLayouts<ENTRY>::tag_type, glsl::push_constant_std430>)
void AddPushConstant::add_push_constant(PushConstantRange const& push_constant_range_out)
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
        std::free(demangled_type);
#endif
        static constexpr int member_index = MemberLayout::member_index;
        add_push_constant_member(std::get<member_index>(layouts));
      }
    }(), ...);
  }(typename decltype(ShaderVariableLayouts<ENTRY>::struct_layout)::members_tuple{});

  // Don't add the same PushConstantRange object twice.
  ASSERT(std::count_if(m_user_push_constant_ranges.begin(), m_user_push_constant_ranges.end(),
        [&](PushConstantRange const* ptr){ return ptr == &push_constant_range_out; }) == 0);
  // Keep a list of PushConstantRange objects that the user wants to use.
  // We need to call set_shader_bit on those later on.
  m_user_push_constant_ranges.push_back(&push_constant_range_out);
}

} // namespace vulkan::pipeline

#endif // VULKAN_PIPELINE_ADD_PUSH_CONSTANT_H_definitions
