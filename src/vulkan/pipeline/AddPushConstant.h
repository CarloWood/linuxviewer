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
  std::vector<PushConstantRange const*> m_user_push_constant_ranges;    // As passed by the user to add_push_constant.
  std::vector<PushConstantRange> m_push_constant_ranges;                // The output vector registered with the FlatCreateInfo object.

  // Override CharacteristicRangeBridge virtual function.
  void reset_push_constant_ranges() override { m_sorted_push_constant_ranges.clear(); }
  // Override AddShaderStageBridge virtual function.
  AddPushConstant* convert_to_add_push_constant() override { return this; }

  //---------------------------------------------------------------------------
  // Push constants.
  using glsl_id_full_to_push_constant_declaration_context_container_t = std::map<std::string, std::unique_ptr<shader_builder::PushConstantDeclarationContext>>;
  glsl_id_full_to_push_constant_declaration_context_container_t m_glsl_id_full_to_push_constant_declaration_context;    // Map the prefix of VertexAttribute::m_glsl_id_full to its
                                                                                                                        // DeclarationContext object.
  using glsl_id_full_to_push_constant_container_t = std::map<std::string, shader_builder::PushConstant, std::less<>>;
  glsl_id_full_to_push_constant_container_t m_glsl_id_full_to_push_constant;                    // Map PushConstant::m_glsl_id_full to the PushConstant object that contains it.

  std::set<PushConstantRange> m_sorted_push_constant_ranges;
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
  void insert_push_constant_range(PushConstantRange const& push_constant_range)
  {
    auto range = m_sorted_push_constant_ranges.equal_range(push_constant_range);
    m_sorted_push_constant_ranges.erase(range.first, range.second);
    m_sorted_push_constant_ranges.insert(push_constant_range);
    m_sorted_push_constant_ranges_changed = true;

    // Also update the stage flags on the user PushConstantRange objects.
    for (PushConstantRange const* user_push_constant_range : m_user_push_constant_ranges)
    {
      // If the user objects range (partially) overlaps with the range that we found to be used in
      // in the shader stage `push_constant_range.shader_stage_flags()` then set that shader stage
      // bit also in the user object. Note: different types never overlap.
      if (user_push_constant_range->type_index() == push_constant_range.type_index() &&
          !(user_push_constant_range->offset() + user_push_constant_range->size() <= push_constant_range.offset() ||
            push_constant_range.offset() + push_constant_range.size() <= user_push_constant_range->offset()))
        user_push_constant_range->set_shader_bit(push_constant_range.shader_stage_flags());
    }
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
  void add_push_constant(PushConstantRange const& push_constant_range_out);
};

} // namespace vulkan::pipeline
