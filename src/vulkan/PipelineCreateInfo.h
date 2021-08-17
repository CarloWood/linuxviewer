#pragma once

#include <vulkan/vulkan.hpp>
#include "debug.h"

namespace vulkan {

struct PipelineCreateInfo
{
  PipelineCreateInfo() = default;
  PipelineCreateInfo(PipelineCreateInfo const&) = delete;
  PipelineCreateInfo& operator=(PipelineCreateInfo const&) = delete;

  vk::Viewport viewport;
  vk::Rect2D scissor;
  vk::PipelineViewportStateCreateInfo viewportInfo;
  vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
  vk::PipelineRasterizationStateCreateInfo rasterizationInfo;
  vk::PipelineMultisampleStateCreateInfo multisampleInfo;
  vk::PipelineColorBlendAttachmentState colorBlendAttachment;
  vk::PipelineColorBlendStateCreateInfo colorBlendInfo;
  vk::PipelineDepthStencilStateCreateInfo depthStencilInfo;
  vk::PipelineLayout pipelineLayout;
  vk::RenderPass renderPass;
  uint32_t subpass = 0;

  void print_on(std::ostream& os) const;
};

} // namespace vulkan
