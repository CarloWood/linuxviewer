#pragma once

#include "descriptor/SetLayout.h"
#include "descriptor/SetIndexHintMap.h"
#include "threadsafe/aithreadsafe.h"
#include "utils/Vector.h"
#include <vulkan/vulkan.hpp>
#include <vector>
#include <functional>
#include "debug.h"

namespace task {
class PipelineFactory;
} // namespace task

namespace vulkan::pipeline {

class FlatCreateInfo
{
 private:
  using pipeline_shader_stage_create_infos_list_t = aithreadsafe::Wrapper<std::vector<std::vector<vk::PipelineShaderStageCreateInfo> const*>, aithreadsafe::policy::Primitive<std::mutex>>;
  pipeline_shader_stage_create_infos_list_t m_pipeline_shader_stage_create_infos_list;

  using vertex_input_binding_descriptions_list_t = aithreadsafe::Wrapper<std::vector<std::vector<vk::VertexInputBindingDescription> const*>, aithreadsafe::policy::Primitive<std::mutex>>;
  vertex_input_binding_descriptions_list_t m_vertex_input_binding_descriptions_list;

  using vertex_input_attribute_descriptions_list_t = aithreadsafe::Wrapper<std::vector<std::vector<vk::VertexInputAttributeDescription> const*>, aithreadsafe::policy::Primitive<std::mutex>>;
  vertex_input_attribute_descriptions_list_t m_vertex_input_attribute_descriptions_list;

  using pipeline_color_blend_attachment_states_list_t = aithreadsafe::Wrapper<std::vector<std::vector<vk::PipelineColorBlendAttachmentState> const*>, aithreadsafe::policy::Primitive<std::mutex>>;
  pipeline_color_blend_attachment_states_list_t m_pipeline_color_blend_attachment_states_list;

  using dynamic_states_list_t = aithreadsafe::Wrapper<std::vector<std::vector<vk::DynamicState> const*>, aithreadsafe::policy::Primitive<std::mutex>>;
  dynamic_states_list_t m_dynamic_states_list;

  using push_constant_ranges_list_t = aithreadsafe::Wrapper<std::vector<std::vector<vk::PushConstantRange> const*>, aithreadsafe::policy::Primitive<std::mutex>>;
  push_constant_ranges_list_t m_push_constant_ranges_list;

  template<typename T>
  static std::vector<T> merge(aithreadsafe::Wrapper<std::vector<std::vector<T> const*>, aithreadsafe::policy::Primitive<std::mutex>> const& input_list)
  {
    std::vector<T> result;
    typename std::remove_reference_t<decltype(input_list)>::crat input_list_r(input_list);
    size_t s = 0;
    for (std::vector<T> const* v : *input_list_r)
    {
      // You called add(std::vector<T> const&) but never filled the passed vector with data.
      // For example, you might have a pipeline factory characteristic that is derived from AddPushConstant
      // and then are not calling add_push_constant<MyPushConstant>() in its initialization state.
      // If you do not have a push constant you should not derive from AddPushConstant.
      ASSERT(v->size() != 0);
      s += v->size();
    }
    result.reserve(s);
    for (std::vector<T> const* v : *input_list_r)
      result.insert(result.end(), v->begin(), v->end());
    return result;
  }

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
  int add(std::vector<vk::PipelineShaderStageCreateInfo> const* pipeline_shader_stage_create_infos)
  {
    pipeline_shader_stage_create_infos_list_t::wat pipeline_shader_stage_create_infos_list_w(m_pipeline_shader_stage_create_infos_list);
    pipeline_shader_stage_create_infos_list_w->push_back(pipeline_shader_stage_create_infos);
    return pipeline_shader_stage_create_infos_list_w->size() - 1;
  }

  std::vector<vk::PipelineShaderStageCreateInfo> get_pipeline_shader_stage_create_infos() const
  {
    return merge(m_pipeline_shader_stage_create_infos_list);
  }

  int add(std::vector<vk::VertexInputBindingDescription> const* vertex_input_binding_descriptions)
  {
    vertex_input_binding_descriptions_list_t::wat vertex_input_binding_descriptions_list_w(m_vertex_input_binding_descriptions_list);
    vertex_input_binding_descriptions_list_w->push_back(vertex_input_binding_descriptions);
    return vertex_input_binding_descriptions_list_w->size() - 1;
  }

  std::vector<vk::VertexInputBindingDescription> get_vertex_input_binding_descriptions() const
  {
    return merge(m_vertex_input_binding_descriptions_list);
  }

  int add(std::vector<vk::VertexInputAttributeDescription> const* vertex_input_attribute_descriptions)
  {
    vertex_input_attribute_descriptions_list_t::wat vertex_input_attribute_descriptions_list_w(m_vertex_input_attribute_descriptions_list);
    vertex_input_attribute_descriptions_list_w->push_back(vertex_input_attribute_descriptions);
    return vertex_input_attribute_descriptions_list_w->size() - 1;
  }

  std::vector<vk::VertexInputAttributeDescription> get_vertex_input_attribute_descriptions() const
  {
    return merge(m_vertex_input_attribute_descriptions_list);
  }

  int add(std::vector<vk::PipelineColorBlendAttachmentState> const* pipeline_color_blend_attachment_states)
  {
    pipeline_color_blend_attachment_states_list_t::wat pipeline_color_blend_attachment_states_list_w(m_pipeline_color_blend_attachment_states_list);
    pipeline_color_blend_attachment_states_list_w->push_back(pipeline_color_blend_attachment_states);
    return pipeline_color_blend_attachment_states_list_w->size() - 1;
  }

  std::vector<vk::PipelineColorBlendAttachmentState> get_pipeline_color_blend_attachment_states() const
  {
    std::vector<vk::PipelineColorBlendAttachmentState> pipeline_color_blend_attachment_states = merge(m_pipeline_color_blend_attachment_states_list);
    // Use add(std::vector<vk::PipelineColorBlendAttachmentState> const& pipeline_color_blend_attachment_states)
    // instead of manipulating m_color_blend_state_create_info.attachmentCount and/or m_color_blend_state_create_info.pAttachments.
    ASSERT(m_color_blend_state_create_info.attachmentCount == 0 && m_color_blend_state_create_info.pAttachments == nullptr);
    m_color_blend_state_create_info.setAttachments(pipeline_color_blend_attachment_states);
    return pipeline_color_blend_attachment_states;
  }

  int add(std::vector<vk::DynamicState> const* dynamic_states)
  {
    dynamic_states_list_t::wat dynamic_states_list_w(m_dynamic_states_list);
    dynamic_states_list_w->push_back(dynamic_states);
    return dynamic_states_list_w->size() - 1;
  }

  std::vector<vk::DynamicState> get_dynamic_states() const
  {
    return merge(m_dynamic_states_list);
  }

  int add(std::vector<vk::PushConstantRange> const* push_constant_ranges)
  {
    push_constant_ranges_list_t::wat push_constant_ranges_list_w(m_push_constant_ranges_list);
    push_constant_ranges_list_w->push_back(push_constant_ranges);
    return push_constant_ranges_list_w->size() - 1;
  }

  std::vector<vk::PushConstantRange> get_sorted_push_constant_ranges() const
  {
    // Merging push constant ranges doesn't seem to make sense; at least it is not supported right now.
    // Only add a std::vector<vk::PushConstantRange> once, from the initialize of a single PipelineCharacteristic.
    ASSERT(push_constant_ranges_list_t::crat(m_push_constant_ranges_list)->size() <= 1);
    // This is returning the only vector that was added (if any), which was already sorted (see AddPushConstant::push_constant_ranges()).
    return merge(m_push_constant_ranges_list);
  }
};

} // namespace vulkan::pipeline
