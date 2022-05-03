#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>
#include "debug.h"

namespace vulkan::pipeline {

class FlatCreateInfo
{
 private:
  std::vector<std::vector<vk::PipelineShaderStageCreateInfo> const*> m_pipeline_shader_stage_create_infos_list;
  std::vector<std::vector<vk::VertexInputBindingDescription> const*> m_vertex_input_binding_descriptions_list;
  std::vector<std::vector<vk::VertexInputAttributeDescription> const*> m_vertex_input_attribute_descriptions_list;
  std::vector<std::vector<vk::PipelineColorBlendAttachmentState> const*> m_pipeline_color_blend_attachment_states_list;
  std::vector<std::vector<vk::DynamicState> const*> m_dynamic_states_list;

  template<typename T>
  static std::vector<T> merge(std::vector<std::vector<T> const*> input_list)
  {
    std::vector<T> result;
    size_t s = 0;
    for (std::vector<T> const* v : input_list)
    {
      // You called add(std::vector<T> const&) but never filled the passed vector with data.
      ASSERT(v->size() != 0);
      s += v->size();
    }
    result.reserve(s);
    for (std::vector<T> const* v : input_list)
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
  int add(std::vector<vk::PipelineShaderStageCreateInfo> const& pipeline_shader_stage_create_infos)
  {
    m_pipeline_shader_stage_create_infos_list.push_back(&pipeline_shader_stage_create_infos);
    return m_pipeline_shader_stage_create_infos_list.size() - 1;
  }

  std::vector<vk::PipelineShaderStageCreateInfo> get_pipeline_shader_stage_create_infos() const
  {
    return merge(m_pipeline_shader_stage_create_infos_list);
  }

  int add(std::vector<vk::VertexInputBindingDescription> const& vertex_input_binding_descriptions)
  {
    m_vertex_input_binding_descriptions_list.push_back(&vertex_input_binding_descriptions);
    return m_vertex_input_binding_descriptions_list.size() - 1;
  }

  std::vector<vk::VertexInputBindingDescription> get_vertex_input_binding_descriptions() const
  {
    return merge(m_vertex_input_binding_descriptions_list);
  }

  int add(std::vector<vk::VertexInputAttributeDescription> const& vertex_input_attribute_descriptions)
  {
    m_vertex_input_attribute_descriptions_list.push_back(&vertex_input_attribute_descriptions);
    return m_vertex_input_attribute_descriptions_list.size() - 1;
  }

  std::vector<vk::VertexInputAttributeDescription> get_vertex_input_attribute_descriptions() const
  {
    return merge(m_vertex_input_attribute_descriptions_list);
  }

  int add(std::vector<vk::PipelineColorBlendAttachmentState> const& pipeline_color_blend_attachment_states)
  {
    m_pipeline_color_blend_attachment_states_list.push_back(&pipeline_color_blend_attachment_states);
    return m_pipeline_color_blend_attachment_states_list.size() - 1;
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

  int add(std::vector<vk::DynamicState> const& dynamic_states)
  {
    m_dynamic_states_list.push_back(&dynamic_states);
    return m_dynamic_states_list.size() - 1;
  }

  std::vector<vk::DynamicState> get_dynamic_states() const
  {
    return merge(m_dynamic_states_list);
  }
};

} // namespace vulkan::pipeline
