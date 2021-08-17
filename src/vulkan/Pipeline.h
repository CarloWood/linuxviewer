#pragma once

#include "Device.h"
#include "PipelineCreateInfo.h"
#include <string>
#include <vector>
#include <cstdint>

namespace vulkan {

class Pipeline
{
 public:
  Pipeline(
      Device const& device,
      std::string const& vertFilepath,
      std::string const& fragFilepath,
      PipelineCreateInfo const& configInfo);
  ~Pipeline();

  Pipeline(Pipeline const&) = delete;
  Pipeline& operator=(Pipeline const&) = delete;

  void bind(vk::CommandBuffer commandBuffer);

  static void defaultPipelineCreateInfo(PipelineCreateInfo& create_info, uint32_t width, uint32_t height);

 private:
  static std::vector<uint32_t> read_SPIRV_file(std::string const& filepath);

  void createGraphicsPipeline(std::string const& vertFilepath, std::string const& fragFilepath, PipelineCreateInfo const& configInfo);
  vk::ShaderModule createShaderModule(std::vector<uint32_t> const& code);

  Device const& m_device;
  vk::Pipeline graphicsPipeline;
  vk::ShaderModule vertShaderModule;
  vk::ShaderModule fragShaderModule;
};

} // namespace vulkan
