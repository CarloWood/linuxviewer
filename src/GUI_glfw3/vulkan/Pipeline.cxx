#include "sys.h"
#include "Pipeline.h"
#include "HelloTriangleDevice.h"
#include "debug.h"
#include <stdexcept>
#include <string>
#include <fstream>

#if defined(CWDEBUG) && !defined(DOXYGEN)
NAMESPACE_DEBUG_CHANNELS_START
channel_ct vulkan("VULKAN");
NAMESPACE_DEBUG_CHANNELS_END
#endif

namespace vulkan {

Pipeline::Pipeline(
    HelloTriangleDevice& device,
    std::string const& vertFilepath,
    std::string const& fragFilepath,
    PipelineCreateInfo const& create_info) : m_device(device)
{
  DoutEntering(dc::vulkan, "Pipeline::Pipeline(" << device << ", \"" << vertFilepath << "\", \"" << fragFilepath << "\", " << create_info << ")");
  createGraphicsPipeline(vertFilepath, fragFilepath, create_info);
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
  auto vertCode = readFile(vertFilepath);
  auto fragCode = readFile(fragFilepath);

  Dout(dc::vulkan, "Vertex Shader Code Size: " << vertCode.size());
  Dout(dc::vulkan, "Fragment Shader Code Size: " << fragCode.size());
}

void Pipeline::createShaderModule(std::vector<char> const& code, VkShaderModule* shaderModule)
{
  VkShaderModuleCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  create_info.codeSize = code.size();
  create_info.pCode = reinterpret_cast<uint32_t const*>(code.data());
  if (vkCreateShaderModule(m_device.device(), &create_info, nullptr, shaderModule) != VK_SUCCESS)
    throw std::runtime_error("Failed to create shader module");
}

PipelineCreateInfo Pipeline::defaultPipelineCreateInfo(uint32_t width, uint32_t height)
{
  PipelineCreateInfo create_info{};

  return create_info;
}

} // namespace vulkan
