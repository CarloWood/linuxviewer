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

  void bind(vk::CommandBuffer vh_command_buffer);

  static void defaultPipelineCreateInfo(PipelineCreateInfo& create_info, uint32_t width, uint32_t height);

 private:
  static std::vector<uint32_t> read_SPIRV_file(std::string const& filepath);

  void createGraphicsPipeline(std::string const& vertFilepath, std::string const& fragFilepath, PipelineCreateInfo const& configInfo);
  vk::ShaderModule createShaderModule(std::vector<uint32_t> const& code);

  Device const& m_device;
  vk::Pipeline m_vh_graphics_pipeline;
  vk::ShaderModule m_vh_vertex_shader_module;
  vk::ShaderModule m_vh_fragment_shader_module;
};

} // namespace vulkan
