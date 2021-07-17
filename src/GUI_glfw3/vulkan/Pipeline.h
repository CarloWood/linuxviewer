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
      HelloTriangleDevice& device,
      std::string const& vertFilepath,
      std::string const& fragFilepath,
      PipelineCreateInfo const& configInfo);
  ~Pipeline() {}

  Pipeline(Pipeline const&) = delete;
  Pipeline& operator=(Pipeline const&) = delete;

  static PipelineCreateInfo defaultPipelineCreateInfo(uint32_t width, uint32_t height);

 private:
  static std::vector<char> readFile(std::string const& filepath);

  void createGraphicsPipeline(std::string const& vertFilepath, std::string const& fragFilepath, PipelineCreateInfo const& configInfo);
  void createShaderModule(std::vector<char> const& code, VkShaderModule* shaderModule);

  HelloTriangleDevice& m_device;
  VkPipeline graphicsPipeline;
  VkShaderModule vertShaderModule;
  VkShaderModule fragShaderModule;
};

} // namespace vulkan

#if defined(CWDEBUG) && !defined(DOXYGEN)
NAMESPACE_DEBUG_CHANNELS_START
extern channel_ct vulkan;
NAMESPACE_DEBUG_CHANNELS_END
#endif
