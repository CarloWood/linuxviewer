#pragma once

#include "HelloTriangleDevice.h"
#include "PipelineCreateInfo.h"
#include <string>
#include <vector>
#include <cstdint>

namespace vulkan {

class Pipeline
{
 public:
  Pipeline(
      VkDevice device_handle,
      std::string const& vertFilepath,
      std::string const& fragFilepath,
      PipelineCreateInfo const& configInfo);
  ~Pipeline();

  Pipeline(Pipeline const&) = delete;
  Pipeline& operator=(Pipeline const&) = delete;

  void bind(VkCommandBuffer commandBuffer);

  static void defaultPipelineCreateInfo(PipelineCreateInfo& create_info, uint32_t width, uint32_t height);

 private:
  static std::vector<char> readFile(std::string const& filepath);

  void createGraphicsPipeline(std::string const& vertFilepath, std::string const& fragFilepath, PipelineCreateInfo const& configInfo);
  void createShaderModule(std::vector<char> const& code, VkShaderModule* shaderModule);

  VkDevice m_device_handle;
  VkPipeline graphicsPipeline;
  VkShaderModule vertShaderModule;
  VkShaderModule fragShaderModule;
};

} // namespace vulkan
