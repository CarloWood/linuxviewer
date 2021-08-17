#include "sys.h"
#include "Pipeline.h"
#include "Device.h"
#include "utils/endian.h"
#include "debug.h"
#include <stdexcept>
#include <string>
#include <fstream>

namespace vulkan {

Pipeline::Pipeline(
    Device const& device,
    std::string const& vertFilepath,
    std::string const& fragFilepath,
    PipelineCreateInfo const& create_info) : m_device(device)
{
  DoutEntering(dc::vulkan, "Pipeline::Pipeline(" << device << ", \"" << vertFilepath << "\", \"" << fragFilepath << "\", " << create_info << ")");
  createGraphicsPipeline(vertFilepath, fragFilepath, create_info);
}

Pipeline::~Pipeline()
{
  m_device->destroyShaderModule(vertShaderModule);
  m_device->destroyShaderModule(fragShaderModule);
  m_device->destroyPipeline(graphicsPipeline);
}

std::vector<uint32_t> Pipeline::read_SPIRV_file(std::string const& filepath)
{
  std::ifstream file{filepath, std::ios::ate | std::ios::binary};

  if (!file.is_open())
    throw std::runtime_error("failed to open file: " + filepath);

  size_t number_of_words = static_cast<size_t>(file.tellg()) / sizeof(uint32_t);
  std::vector<uint32_t> buffer(number_of_words);

  file.seekg(0);
  file.read(reinterpret_cast<char*>(buffer.data()), number_of_words * sizeof(uint32_t));
  file.close();

  // Convert little endian to host order.
  for (auto& word : buffer)
    word = le_to_uint32(reinterpret_cast<char const*>(&word));

  return buffer;
}

void Pipeline::createGraphicsPipeline(
    std::string const& vertFilepath, std::string const& fragFilepath, PipelineCreateInfo const& create_info)
{
  ASSERT(create_info.pipelineLayout &&
      "Cannot create graphics pipeline: no pipelineLayout provided in create_info");
  ASSERT(create_info.renderPass &&
      "Cannot create graphics pipeline: no renderPass provided in create_info");

  auto vertCode = read_SPIRV_file(vertFilepath);
  auto fragCode = read_SPIRV_file(fragFilepath);

  vertShaderModule = createShaderModule(vertCode);
  fragShaderModule = createShaderModule(fragCode);

  std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = {{
    { {}, vk::ShaderStageFlagBits::eVertex, vertShaderModule, "main", nullptr },
    { {}, vk::ShaderStageFlagBits::eFragment, fragShaderModule, "main", nullptr },
  }};

  vk::PipelineVertexInputStateCreateInfo vertexInputInfo;

  vk::GraphicsPipelineCreateInfo pipelineInfo;
  pipelineInfo
    .setStages(shaderStages)
    .setPVertexInputState(&vertexInputInfo)
    .setPInputAssemblyState(&create_info.inputAssemblyInfo)
    .setPViewportState(&create_info.viewportInfo)
    .setPRasterizationState(&create_info.rasterizationInfo)
    .setPMultisampleState(&create_info.multisampleInfo)
    .setPColorBlendState(&create_info.colorBlendInfo)
    .setPDepthStencilState(&create_info.depthStencilInfo)
    .setLayout(create_info.pipelineLayout)
    .setRenderPass(create_info.renderPass)
    .setSubpass(create_info.subpass)
    .setBasePipelineIndex(-1)
    ;

  auto rv = m_device->createGraphicsPipelines({}, pipelineInfo);
  if (rv.result != vk::Result::eSuccess)
    throw std::runtime_error("failed to create graphics pipeline");

  graphicsPipeline = rv.value[0];
}

vk::ShaderModule Pipeline::createShaderModule(std::vector<uint32_t> const& code)
{
  vk::ShaderModuleCreateInfo create_info;
  create_info
    .setCode(code)
    ;
  return m_device->createShaderModule(create_info);
}

void Pipeline::bind(vk::CommandBuffer commandBuffer)
{
  commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);
}

void Pipeline::defaultPipelineCreateInfo(PipelineCreateInfo& create_info, uint32_t width, uint32_t height)
{
  create_info.inputAssemblyInfo
    .setTopology(vk::PrimitiveTopology::eTriangleList)
    .setPrimitiveRestartEnable(false)
    ;

  create_info.viewport
    .setX(0.0f)
    .setY(0.0f)
    .setWidth(width)
    .setHeight(height)
    .setMinDepth(0.0f)
    .setMaxDepth(1.0f)
    ;

  create_info.scissor
    .setOffset({ 0, 0 })
    .setExtent({ width, height })
    ;

  create_info.viewportInfo
    .setViewports(create_info.viewport)
    .setScissors(create_info.scissor)
    ;

  create_info.rasterizationInfo
    .setDepthClampEnable(false)
    .setRasterizerDiscardEnable(false)
    .setPolygonMode(vk::PolygonMode::eFill)
    .setLineWidth(1.0f)
    .setCullMode(vk::CullModeFlagBits::eNone)
    .setFrontFace(vk::FrontFace::eCounterClockwise)
    .setDepthBiasEnable(false)
    .setDepthBiasConstantFactor(0.0f)
    .setDepthBiasClamp(0.0f)
    .setDepthBiasSlopeFactor(0.0f)
    ;

  create_info.rasterizationInfo
    .setRasterizerDiscardEnable(false)
    .setPolygonMode(vk::PolygonMode::eFill)
    .setLineWidth(1.0f)
    .setCullMode(vk::CullModeFlagBits::eNone)
    .setFrontFace(vk::FrontFace::eCounterClockwise)
    .setDepthBiasEnable(false)
    .setDepthBiasConstantFactor(0.0f)
    .setDepthBiasClamp(0.0f)
    .setDepthBiasSlopeFactor(0.0f)
    ;

  create_info.multisampleInfo
    .setSampleShadingEnable(false)
    .setRasterizationSamples(vk::SampleCountFlagBits::e1)
    .setMinSampleShading(1.0f)
    .setPSampleMask(nullptr)
    .setAlphaToCoverageEnable(false)
    .setAlphaToOneEnable(false)
    ;

  create_info.colorBlendAttachment
    .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
    .setBlendEnable(false)
    .setSrcColorBlendFactor(vk::BlendFactor::eOne)
    .setDstColorBlendFactor(vk::BlendFactor::eZero)
    .setColorBlendOp(vk::BlendOp::eAdd)
    .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
    .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
    .setAlphaBlendOp(vk::BlendOp::eAdd)
    ;

  create_info.colorBlendInfo
    .setLogicOpEnable(false)
    .setLogicOp(vk::LogicOp::eCopy)
    .setAttachments(create_info.colorBlendAttachment)
    .setBlendConstants({ 0.0f, 0.0f, 0.0f, 0.0f })
    ;

  create_info.depthStencilInfo
    .setDepthTestEnable(true)
    .setDepthWriteEnable(false)
    .setDepthCompareOp(vk::CompareOp::eLess)
    .setDepthBoundsTestEnable(false)
    .setMinDepthBounds(0.0f)
    .setMaxDepthBounds(1.0f)
    .setStencilTestEnable(false)
    .setFront({})
    .setBack({})
    ;
}

} // namespace vulkan
