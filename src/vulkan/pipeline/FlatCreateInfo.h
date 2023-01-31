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

namespace task {
class PipelineFactory;
} // namespace task

namespace vulkan::pipeline {

namespace {
struct NoPushConstant { };
} // namespace

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

  using push_constant_ranges_list_t = aithreadsafe::Wrapper<std::vector<std::vector<vulkan::PushConstantRange> const*>, aithreadsafe::policy::Primitive<std::mutex>>;
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
      //
      // For example,
      //
      // T = vk::VertexInputBindingDescription:
      // if you do not have any vertex buffers then you should set m_use_vertex_buffers = false; in
      // the constructor of the pipeline factory characteristic derived from AddVertexShader.
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

  int add(std::vector<vulkan::PushConstantRange> const* push_constant_ranges)
  {
    push_constant_ranges_list_t::wat push_constant_ranges_list_w(m_push_constant_ranges_list);
    push_constant_ranges_list_w->push_back(push_constant_ranges);
    return push_constant_ranges_list_w->size() - 1;
  }

  std::vector<vk::PushConstantRange> get_sorted_push_constant_ranges() const
  {
    DoutEntering(dc::vulkan, "FlatCreateInfo::get_sorted_push_constant_ranges() [" << this << "]");

    // sorted_push_constant_ranges is sorted on (from highest to lowest priority):
    // - push constant type (aka, MyPushConstant1, MyPushConstant2, etc).
    // - vk::ShaderStageFlags
    // - offset
    //
    // Merging then works as follows:
    // 1) each push constant type is unique, and just catenated in no particular order (lets say in the order
    //    of whatever the id is that we assign to each type).
    // 2) ranges for a given vk::ShaderStageFlags are extended (offset and size)
    // 3) the new offsets determine the order in which the ranges for each vk::ShaderStageFlags are stored.

    // Original input.
    push_constant_ranges_list_t::crat push_constant_ranges_list_r(m_push_constant_ranges_list);

#if CWDEBUG
    Dout(dc::vulkan, "m_push_constant_ranges_list = ");
    int i = 0;
    for (std::vector<vulkan::PushConstantRange> const* vec_ptr : *push_constant_ranges_list_r)
    {
      Dout(dc::vulkan, "  ranges " << i << ":");
      for (vulkan::PushConstantRange const& range : *vec_ptr)
        Dout(dc::vulkan, "    " << range);
    }
#endif

    // If there is input data, return an empty vector.
    if (push_constant_ranges_list_r->empty())
      return {};

    // Intermediate vectors.
    std::array<std::vector<std::vector<vulkan::PushConstantRange>>, 2> storage;

    // Copy m_push_constant_ranges_list into storage[0].
    int number_of_input_vectors = push_constant_ranges_list_r->size();
    storage[0].resize(number_of_input_vectors);
    for (int i = 0; i < number_of_input_vectors; ++i)
      std::copy((*push_constant_ranges_list_r)[i]->begin(), (*push_constant_ranges_list_r)[i]->end(), std::back_inserter(storage[0][i]));

    std::vector<std::vector<vulkan::PushConstantRange>> const* input = &storage[0];
    std::vector<std::vector<vulkan::PushConstantRange>>* output = &storage[1];
    int s = 1;  // The index into storage that output is pointing at.

    for (;;)
    {
      auto in_vector1 = input->begin();         // The next vector in the input that wasn't merged yet.
      size_t need_merge = input->size();        // Number of vectors in input that still need merging.
      do
      {
        // The next output vector.
        if (need_merge == 1)
        {
          output->push_back(std::move(*in_vector1));
          break;
        }
        else
        {
          output->emplace_back();
          auto in_vector2 = in_vector1++;
          std::set_union(in_vector2->begin(), in_vector2->end(),
                         in_vector1->begin(), in_vector1->end(),
                         std::back_inserter(output->back()));
          need_merge -= 2;
          ++in_vector1;
        }
      }
      while (need_merge > 0);

      // If everything was merged into a single vector then we're done.
      if (output->size() == 1)
        break;

      input = output;
      s = 1 - s;
      output = &storage[s];
      output->clear();
    }

    // Make a reference to the consolidated data in output.
    std::vector<vulkan::PushConstantRange> const& consolidated_ranges = (*output)[0];

    std::vector<vk::PushConstantRange> merged_result;

    std::type_index last_type_index = typeid(NoPushConstant);
    vk::ShaderStageFlags last_shader_stage_flags{};
#ifdef CWDEBUG
    int last_gap = 0;
    int last_size = 1;
#endif

    for (auto in = consolidated_ranges.begin(); in != consolidated_ranges.end(); ++in)
    {
      if (in->type_index() != last_type_index || in->shader_stage_flags() != last_shader_stage_flags)
      {
#ifdef CWDEBUG
        int unused_gap = 100 * last_gap / last_size;
        if (unused_gap > 20)
          Dout(dc::warning, "Push constant range is nonconsecutive (" << unused_gap << "%% is unused).");
        // Please group the members of your PushConstant structs so that those that are used in the same stage(s) are together.
        ASSERT(unused_gap < 50);
#endif
        merged_result.emplace_back(in->shader_stage_flags(), in->offset(), in->size());
        last_type_index = in->type_index();
        last_shader_stage_flags = in->shader_stage_flags();
#ifdef CWDEBUG
        last_gap = 0;
        last_size = in->size();
#endif
      }
      else
      {
        vk::PushConstantRange& last_result = merged_result.back();
        // Note: no need to update last_result.offset because consolidated_ranges was sorted on offset already,
        // therefore last_result.offset will be the smallest value already.
        //
#ifdef CWDEBUG
        // The resulting range only becomes nonconsecutive if the next range has an offset that is beyond the current range.
        last_gap += std::max(uint32_t{0}, in->offset() - (last_result.offset + last_result.size));
#endif
        last_result.size = std::max(last_result.offset + last_result.size, in->offset() + in->size()) - last_result.offset;
#ifdef CWDEBUG
        last_size = last_result.size;
#endif
      }
    }

    return merged_result;
  }
};

} // namespace vulkan::pipeline
