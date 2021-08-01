#include "sys.h"
#include "Pipeline.h"
#include "HelloTriangleDevice.h"
#include "debug.h"
#include <stdexcept>
#include <string>
#include <fstream>

namespace vulkan {

Pipeline::Pipeline(
    VkDevice device_handle,
    std::string const& vertFilepath,
    std::string const& fragFilepath,
    PipelineCreateInfo const& create_info) : m_device_handle(device_handle)
{
  DoutEntering(dc::vulkan, "Pipeline::Pipeline(" << device_handle << ", \"" << vertFilepath << "\", \"" << fragFilepath << "\", " << create_info << ")");
  createGraphicsPipeline(vertFilepath, fragFilepath, create_info);
}

Pipeline::~Pipeline()
{
  vkDestroyShaderModule(m_device_handle, vertShaderModule, nullptr);
  vkDestroyShaderModule(m_device_handle, fragShaderModule, nullptr);
  vkDestroyPipeline(m_device_handle, graphicsPipeline, nullptr);
}

std::vector<char> Pipeline::readFile(std::string const& filepath)
{
  std::ifstream file{filepath, std::ios::ate | std::ios::binary};

  if (!file.is_open())
    throw std::runtime_error("failed to open file: " + filepath);

  size_t fileSize = static_cast<size_t>(file.tellg());
  std::vector<char> buffer(fileSize);

  file.seekg(0);
  file.read(buffer.data(), fileSize);

  file.close();
  return buffer;
}

void Pipeline::createGraphicsPipeline(
    std::string const& vertFilepath, std::string const& fragFilepath, PipelineCreateInfo const& create_info)
{
  ASSERT(create_info.pipelineLayout != VK_NULL_HANDLE &&
      "Cannot create graphics pipeline: no pipelineLayout provided in create_info");
  ASSERT(create_info.renderPass != VK_NULL_HANDLE &&
      "Cannot create graphics pipeline: no renderPass provided in create_info");

  auto vertCode = readFile(vertFilepath);
  auto fragCode = readFile(fragFilepath);

  createShaderModule(vertCode, &vertShaderModule);
  createShaderModule(fragCode, &fragShaderModule);

  VkPipelineShaderStageCreateInfo shaderStages[2];
  shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  shaderStages[0].module = vertShaderModule;
  shaderStages[0].pName = "main";
  shaderStages[0].flags = 0;
  shaderStages[0].pNext = nullptr;
  shaderStages[0].pSpecializationInfo = nullptr;
  shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  shaderStages[1].module = fragShaderModule;
  shaderStages[1].pName = "main";
  shaderStages[1].flags = 0;
  shaderStages[1].pNext = nullptr;
  shaderStages[1].pSpecializationInfo = nullptr;

  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexAttributeDescriptionCount = 0;
  vertexInputInfo.vertexBindingDescriptionCount = 0;
  vertexInputInfo.pVertexAttributeDescriptions = nullptr;
  vertexInputInfo.pVertexBindingDescriptions = nullptr;

  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &create_info.inputAssemblyInfo;
  pipelineInfo.pViewportState = &create_info.viewportInfo;
  pipelineInfo.pRasterizationState = &create_info.rasterizationInfo;
  pipelineInfo.pMultisampleState = &create_info.multisampleInfo;
  pipelineInfo.pColorBlendState = &create_info.colorBlendInfo;
  pipelineInfo.pDepthStencilState = &create_info.depthStencilInfo;
  pipelineInfo.pDynamicState = nullptr;

  pipelineInfo.layout = create_info.pipelineLayout;
  pipelineInfo.renderPass = create_info.renderPass;
  pipelineInfo.subpass = create_info.subpass;

  pipelineInfo.basePipelineIndex = -1;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

  if (vkCreateGraphicsPipelines(m_device_handle, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
    throw std::runtime_error("failed to create graphics pipeline");
}

void Pipeline::createShaderModule(std::vector<char> const& code, VkShaderModule* shaderModule)
{
  VkShaderModuleCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  create_info.codeSize = code.size();
  create_info.pCode = reinterpret_cast<uint32_t const*>(code.data());
  if (vkCreateShaderModule(m_device_handle, &create_info, nullptr, shaderModule) != VK_SUCCESS)
    throw std::runtime_error("Failed to create shader module");
}

void Pipeline::bind(VkCommandBuffer commandBuffer)
{
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
}

void Pipeline::defaultPipelineCreateInfo(PipelineCreateInfo& create_info, uint32_t width, uint32_t height)
{
  create_info.inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  create_info.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  create_info.inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

  create_info.viewport.x = 0.0f;
  create_info.viewport.y = 0.0f;
  create_info.viewport.width = static_cast<float>(width);
  create_info.viewport.height = static_cast<float>(height);
  create_info.viewport.minDepth = 0.0f;
  create_info.viewport.maxDepth = 1.0f;

  create_info.scissor.offset = { 0, 0 };
  create_info.scissor.extent = { width, height };

  create_info.viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  create_info.viewportInfo.viewportCount = 1;
  create_info.viewportInfo.pViewports = &create_info.viewport;
  create_info.viewportInfo.scissorCount = 1;
  create_info.viewportInfo.pScissors = &create_info.scissor;

  create_info.rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  create_info.rasterizationInfo.depthClampEnable = VK_FALSE;
  create_info.rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
  create_info.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
  create_info.rasterizationInfo.lineWidth = 1.0f;
  create_info.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
  create_info.rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
  create_info.rasterizationInfo.depthBiasEnable = VK_FALSE;
  create_info.rasterizationInfo.depthBiasConstantFactor = 0.0f;
  create_info.rasterizationInfo.depthBiasClamp = 0.0f;
  create_info.rasterizationInfo.depthBiasSlopeFactor = 0.0f;

  create_info.multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  create_info.multisampleInfo.sampleShadingEnable = VK_FALSE;
  create_info.multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  create_info.multisampleInfo.minSampleShading = 1.0f;
  create_info.multisampleInfo.pSampleMask = nullptr;
  create_info.multisampleInfo.alphaToCoverageEnable = VK_FALSE;
  create_info.multisampleInfo.alphaToOneEnable = VK_FALSE;

  create_info.colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  create_info.colorBlendAttachment.blendEnable = VK_FALSE;
  create_info.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
  create_info.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
  create_info.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  create_info.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  create_info.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  create_info.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

  create_info.colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  create_info.colorBlendInfo.logicOpEnable = VK_FALSE;
  create_info.colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
  create_info.colorBlendInfo.attachmentCount = 1;
  create_info.colorBlendInfo.pAttachments = &create_info.colorBlendAttachment;
  create_info.colorBlendInfo.blendConstants[0] = 0.0f;
  create_info.colorBlendInfo.blendConstants[1] = 0.0f;
  create_info.colorBlendInfo.blendConstants[2] = 0.0f;
  create_info.colorBlendInfo.blendConstants[3] = 0.0f;

  create_info.depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  create_info.depthStencilInfo.depthTestEnable = VK_TRUE;
  create_info.depthStencilInfo.depthWriteEnable = VK_FALSE;
  create_info.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
  create_info.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
  create_info.depthStencilInfo.minDepthBounds = 0.0f;
  create_info.depthStencilInfo.maxDepthBounds = 1.0f;
  create_info.depthStencilInfo.stencilTestEnable = VK_FALSE;
  create_info.depthStencilInfo.front = {};
  create_info.depthStencilInfo.back = {};
}

} // namespace vulkan
