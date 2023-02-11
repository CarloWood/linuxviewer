#pragma once

#include "PushConstantRange.h"
#include "descriptor/SetLayout.h"
#include "descriptor/SetIndexHintMap.h"
#include "threadsafe/aithreadsafe.h"
#include "utils/Vector.h"
#include <vulkan/vulkan.hpp>
#include <vector>
#include <functional>
#include <algorithm>
#include "debug.h"

namespace vulkan {

namespace task {
class CharacteristicRange;
} // namespace vulkan::task

namespace pipeline {
using CharacteristicRangeIndex = utils::VectorIndex<task::CharacteristicRange>;

namespace {
struct NoPushConstant { };
} // namespace

template<typename T>
struct CharacteristicDataCache
{
  std::vector<T> const* m_characteristic_data{};
  std::vector<std::vector<T>> m_per_fill_index_cache;
};

class FlatCreateInfo
{
 private:
  // Same type as in PipelineFactory.
  using characteristics_container_t = utils::Vector<boost::intrusive_ptr<task::CharacteristicRange>, pipeline::CharacteristicRangeIndex>;

  using pipeline_shader_stage_create_infos_list_t = aithreadsafe::Wrapper<utils::Vector<CharacteristicDataCache<vk::PipelineShaderStageCreateInfo>, CharacteristicRangeIndex>, aithreadsafe::policy::Primitive<std::mutex>>;
  pipeline_shader_stage_create_infos_list_t m_pipeline_shader_stage_create_infos_list;

  using vertex_input_binding_descriptions_list_t = aithreadsafe::Wrapper<utils::Vector<CharacteristicDataCache<vk::VertexInputBindingDescription>, CharacteristicRangeIndex>, aithreadsafe::policy::Primitive<std::mutex>>;
  vertex_input_binding_descriptions_list_t m_vertex_input_binding_descriptions_list;

  using vertex_input_attribute_descriptions_list_t = aithreadsafe::Wrapper<utils::Vector<CharacteristicDataCache<vk::VertexInputAttributeDescription>, CharacteristicRangeIndex>, aithreadsafe::policy::Primitive<std::mutex>>;
  vertex_input_attribute_descriptions_list_t m_vertex_input_attribute_descriptions_list;

  using pipeline_color_blend_attachment_states_list_t = aithreadsafe::Wrapper<utils::Vector<CharacteristicDataCache<vk::PipelineColorBlendAttachmentState>, CharacteristicRangeIndex>, aithreadsafe::policy::Primitive<std::mutex>>;
  pipeline_color_blend_attachment_states_list_t m_pipeline_color_blend_attachment_states_list;

  using dynamic_states_list_t = aithreadsafe::Wrapper<utils::Vector<CharacteristicDataCache<vk::DynamicState>, CharacteristicRangeIndex>, aithreadsafe::policy::Primitive<std::mutex>>;
  dynamic_states_list_t m_dynamic_states_list;

  using unlocked_push_constant_ranges_list_t = utils::Vector<CharacteristicDataCache<vulkan::PushConstantRange>,                         CharacteristicRangeIndex>;
  using push_constant_ranges_list_t = aithreadsafe::Wrapper<unlocked_push_constant_ranges_list_t, aithreadsafe::policy::Primitive<std::mutex>>;
  push_constant_ranges_list_t m_push_constant_ranges_list;

  template<typename T>
  static std::vector<T> merge(aithreadsafe::Wrapper<utils::Vector<CharacteristicDataCache<T>, CharacteristicRangeIndex>, aithreadsafe::policy::Primitive<std::mutex>>& input_list, characteristics_container_t const& characteristics);

 public:
  vk::PipelineInputAssemblyStateCreateInfo m_pipeline_input_assembly_state_create_info;

  vk::PipelineViewportStateCreateInfo m_viewport_state_create_info{
    .viewportCount = 1,                                 // uint32_t
    .pViewports = nullptr,                              // vk::Viewport const*
    .scissorCount = 1,                                  // uint32_t
    .pScissors = nullptr                                // vk::Rect2D const*
  };

  vk::PipelineRasterizationStateCreateInfo m_rasterization_state_create_info{
    .depthClampEnable = VK_FALSE,                       // vk::Bool32
    .rasterizerDiscardEnable = VK_FALSE,                // vk::Bool32
    .polygonMode = vk::PolygonMode::eFill,              // vk::PolygonMode
    .cullMode = vk::CullModeFlagBits::eBack,            // vk::CullModeFlags
    .frontFace = vk::FrontFace::eCounterClockwise,      // vk::FrontFace
    .depthBiasEnable = VK_FALSE,                        // vk::Bool32
    .depthBiasConstantFactor = 0.0f,                    // float
    .depthBiasClamp = 0.0f,                             // float
    .depthBiasSlopeFactor = 0.0f,                       // float
    .lineWidth = 1.0f                                   // float
  };

  vk::PipelineMultisampleStateCreateInfo m_multisample_state_create_info{
    .rasterizationSamples = vk::SampleCountFlagBits::e1,// vk::SampleCountFlagBits
    .sampleShadingEnable = VK_FALSE,                    // vk::Bool32
    .minSampleShading = 1.0f,                           // float
    .pSampleMask = nullptr,                             // vk::SampleMask const*
    .alphaToCoverageEnable = VK_FALSE,                  // vk::Bool32
    .alphaToOneEnable = VK_FALSE                        // vk::Bool32
  };

  vk::PipelineDepthStencilStateCreateInfo m_depth_stencil_state_create_info{
    .depthTestEnable = VK_TRUE,                         // vk::Bool32
    .depthWriteEnable = VK_TRUE,                        // vk::Bool32
    .depthCompareOp = vk::CompareOp::eLessOrEqual,      // vk::CompareOp
    .depthBoundsTestEnable = {},                        // vk::Bool32
    .stencilTestEnable     = {},                        // vk::Bool32
    .front                 = {},                        // vk::StencilOpState
    .back                  = {},                        // vk::StencilOpState
    .minDepthBounds        = {},                        // float
    .maxDepthBounds        = {}                         // float
  };

  // mutable because this object changes .attachmentCount and .pAttachments when unflattening.
  mutable vk::PipelineColorBlendStateCreateInfo m_color_blend_state_create_info{
    .logicOpEnable = VK_FALSE,                          // vk::Bool32
    .logicOp = vk::LogicOp::eCopy,                      // vk::LogicOp
    .blendConstants = {}                                // vk::ArrayWrapper1D<float, 4>
  };

 public:
  void increment_number_of_characteristics();

  void add(std::vector<vk::PipelineShaderStageCreateInfo> const* pipeline_shader_stage_create_infos, task::CharacteristicRange const& owning_characteristic_range);

  std::vector<vk::PipelineShaderStageCreateInfo> realize_pipeline_shader_stage_create_infos(characteristics_container_t const& characteristics)
  {
    return merge(m_pipeline_shader_stage_create_infos_list, characteristics);
  }

  void add(std::vector<vk::VertexInputBindingDescription> const* vertex_input_binding_descriptions, task::CharacteristicRange const& owning_characteristic_range);

  std::vector<vk::VertexInputBindingDescription> realize_vertex_input_binding_descriptions(characteristics_container_t const& characteristics)
  {
    return merge(m_vertex_input_binding_descriptions_list, characteristics);
  }

  void add(std::vector<vk::VertexInputAttributeDescription> const* vertex_input_attribute_descriptions, task::CharacteristicRange const& owning_characteristic_range);

  std::vector<vk::VertexInputAttributeDescription> realize_vertex_input_attribute_descriptions(characteristics_container_t const& characteristics)
  {
    return merge(m_vertex_input_attribute_descriptions_list, characteristics);
  }

  void add(std::vector<vk::PipelineColorBlendAttachmentState> const* pipeline_color_blend_attachment_states, task::CharacteristicRange const& owning_characteristic_range);

  std::vector<vk::PipelineColorBlendAttachmentState> realize_pipeline_color_blend_attachment_states(characteristics_container_t const& characteristics)
  {
    std::vector<vk::PipelineColorBlendAttachmentState> pipeline_color_blend_attachment_states;
    pipeline_color_blend_attachment_states = merge(m_pipeline_color_blend_attachment_states_list, characteristics);
    // Use add(std::vector<vk::PipelineColorBlendAttachmentState> const& pipeline_color_blend_attachment_states)
    // instead of manipulating m_color_blend_state_create_info.attachmentCount and/or m_color_blend_state_create_info.pAttachments.
    ASSERT(m_color_blend_state_create_info.attachmentCount == 0 && m_color_blend_state_create_info.pAttachments == nullptr);
    // Note: this call stores pipeline_color_blend_attachment_states.data(), but that should
    // remain valid upon returning from this function since the caller either move-constructs
    // from the returned value or even copy-elision applies.
    m_color_blend_state_create_info.setAttachments(pipeline_color_blend_attachment_states);
    return pipeline_color_blend_attachment_states;
  }

  void add(std::vector<vk::DynamicState> const* dynamic_states, task::CharacteristicRange const& owning_characteristic_range);

  std::vector<vk::DynamicState> realize_dynamic_states(characteristics_container_t const& characteristics)
  {
    return merge(m_dynamic_states_list, characteristics);
  }

  void add(std::vector<vulkan::PushConstantRange> const* push_constant_ranges, task::CharacteristicRange const& owning_characteristic_range);

  std::vector<vk::PushConstantRange> realize_sorted_push_constant_ranges(characteristics_container_t const& characteristics);
};

} // namespace pipeline
} // namespace vulkan
