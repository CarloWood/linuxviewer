#pragma once

#include <vulkan/vulkan.h>
#include "debug.h"

namespace vulkan {

struct PipelineCreateInfo
{
  PipelineCreateInfo() = default;
  PipelineCreateInfo(PipelineCreateInfo const&) = delete;
  PipelineCreateInfo& operator=(PipelineCreateInfo const&) = delete;

  VkViewport viewport;
  VkRect2D scissor;
  VkPipelineViewportStateCreateInfo viewportInfo;
  VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
  VkPipelineRasterizationStateCreateInfo rasterizationInfo;
  VkPipelineMultisampleStateCreateInfo multisampleInfo;
  VkPipelineColorBlendAttachmentState colorBlendAttachment;
  VkPipelineColorBlendStateCreateInfo colorBlendInfo;
  VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
  VkPipelineLayout pipelineLayout = nullptr;
  VkRenderPass renderPass = nullptr;
  uint32_t subpass = 0;

  void print_on(std::ostream& os) const;
};

} // namespace vulkan
