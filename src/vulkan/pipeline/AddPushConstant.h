#pragma once

#include "PushConstantRange.h"
#include "CharacteristicRangeBridge.h"
#include "AddShaderStageBridge.h"
#include "shader_builder/PushConstant.h"
#include "shader_builder/ShaderVariableLayouts.h"
#include "shader_builder/PushConstantDeclarationContext.h"
#include "utils/Badge.h"
#include <vulkan/vulkan.hpp>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <concepts>

namespace vukan::task {
class PipelineFactory;
} // namespace vulkan::task

namespace vulkan::pipeline {

class AddPushConstant : public virtual CharacteristicRangeBridge, public virtual AddShaderStageBridge
{
 private:
  AddPushConstant* convert_to_add_push_constant() override { return this; }

  std::vector<vulkan::PushConstantRange> m_push_constant_ranges;

  //---------------------------------------------------------------------------
  // Push constants.
  using glsl_id_full_to_push_constant_declaration_context_container_t = std::map<std::string, std::unique_ptr<shader_builder::PushConstantDeclarationContext>>;
  glsl_id_full_to_push_constant_declaration_context_container_t m_glsl_id_full_to_push_constant_declaration_context;    // Map the prefix of VertexAttribute::m_glsl_id_full to its
                                                                                                                        // DeclarationContext object.
  using glsl_id_full_to_push_constant_container_t = std::map<std::string, shader_builder::PushConstant, std::less<>>;
  glsl_id_full_to_push_constant_container_t m_glsl_id_full_to_push_constant;                    // Map PushConstant::m_glsl_id_full to the PushConstant object that contains it.

  std::set<vulkan::PushConstantRange> m_sorted_push_constant_ranges;
  bool m_sorted_push_constant_ranges_changed{false};

  // Add shader variable (PushConstant) to m_glsl_id_full_to_push_constant (and a pointer to that to m_shader_variables),
  // and a declaration context for its prefix) if that doesn't already exist, for a non-array push constant.
  // Called by add_push_constant.
  template<typename ContainingClass, glsl::Standard Standard, glsl::ScalarIndex ScalarIndex, int Rows, int Cols, size_t Alignment, size_t Size, size_t ArrayStride,
      int MemberIndex, size_t MaxAlignment, size_t Offset, utils::TemplateStringLiteral GlslIdStr>
  void add_push_constant_member(shader_builder::MemberLayout<ContainingClass,
      shader_builder::BasicTypeLayout<Standard, ScalarIndex, Rows, Cols, Alignment, Size, ArrayStride>,
      MemberIndex, MaxAlignment, Offset, GlslIdStr> const& member_layout);

  // Add shader variable (PushConstant) to m_glsl_id_full_to_push_constant (and a pointer to that to m_shader_variables),
  // and a declaration context for its prefix) if that doesn't already exist, for a push constant array.
  // Called by add_push_constant.
  template<typename ContainingClass, glsl::Standard Standard, glsl::ScalarIndex ScalarIndex, int Rows, int Cols, size_t Alignment, size_t Size, size_t ArrayStride,
      int MemberIndex, size_t MaxAlignment, size_t Offset, utils::TemplateStringLiteral GlslIdStr, size_t Elements>
  void add_push_constant_member(shader_builder::MemberLayout<ContainingClass,
      shader_builder::ArrayLayout<shader_builder::BasicTypeLayout<Standard, ScalarIndex, Rows, Cols, Alignment, Size, ArrayStride>, Elements>,
      MemberIndex, MaxAlignment, Offset, GlslIdStr> const& member_layout);

  // Override of CharacteristicRangeBridge.
  void copy_push_constant_ranges(task::PipelineFactory* pipeline_factory) final;
  void register_AddPushConstant_with(task::PipelineFactory* pipeline_factory, task::CharacteristicRange const& characteristic_range) const final;

 public:
  // Called from PushConstantDeclarationContext::glsl_id_full_is_used_in.
  void insert_push_constant_range(vulkan::PushConstantRange const& push_constant_range)
  {
    auto range = m_sorted_push_constant_ranges.equal_range(push_constant_range);
    m_sorted_push_constant_ranges.erase(range.first, range.second);
    m_sorted_push_constant_ranges.insert(push_constant_range);
    m_sorted_push_constant_ranges_changed = true;
  }

  // Used by PushConstant::is_used_in to look up the declaration context.
  glsl_id_full_to_push_constant_declaration_context_container_t const& glsl_id_full_to_push_constant_declaration_context(utils::Badge<shader_builder::PushConstant>) const
  {
    return m_glsl_id_full_to_push_constant_declaration_context;
  }
  glsl_id_full_to_push_constant_container_t const& glsl_id_full_to_push_constant() const { return m_glsl_id_full_to_push_constant; }

 protected:
  template<typename ENTRY>
  requires (std::same_as<typename shader_builder::ShaderVariableLayouts<ENTRY>::tag_type, glsl::push_constant_std430>)
  void add_push_constant();
};

} // namespace vulkan::pipeline
