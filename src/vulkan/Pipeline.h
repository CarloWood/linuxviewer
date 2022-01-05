#pragma once

#include "SynchronousWindow.h"
#include <vulkan/vulkan.hpp>
#include <vector>

namespace vulkan {

class Pipeline
{
 private:
  task::SynchronousWindow const* m_owning_window;
  std::vector<vk::PipelineShaderStageCreateInfo> m_shader_stage_create_infos;
  std::vector<vk::UniqueShaderModule> m_unique_handles;

 public:
  // Construct a Pipeline for a given window.
  Pipeline(task::SynchronousWindow const* owning_window) : m_owning_window(owning_window) { }

  // Add a shader module to this pipeline.
  void add(shaderbuilder::ShaderModule const& shader_module)
  {
    m_unique_handles.push_back(shader_module.create({}, m_owning_window));
    m_shader_stage_create_infos.push_back(
      {
        .flags = vk::PipelineShaderStageCreateFlags(0),
        .stage = shader_module.stage(),
        .module = *m_unique_handles.back(),
        .pName = "main"
      }
    );
  }

  //FIXME: these accessors should not be needed in the end.
  std::vector<vk::PipelineShaderStageCreateInfo> const& shader_stage_create_infos() const { return m_shader_stage_create_infos; }
};

} // namespace vulkan
