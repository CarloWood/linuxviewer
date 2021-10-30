#pragma once

#include "DescriptorSetParameters.h"
#include "ImageParameters.h"
#include "BufferParameters.h"

// FIXME: move to user application.
struct SampleParameters
{
  static constexpr int s_max_objects_count = 1000;
  static constexpr int s_quad_tessellation = 40;

  int                                         ObjectsCount;
  int                                         PreSubmitCpuWorkTime;
  int                                         PostSubmitCpuWorkTime;
  float                                       FrameGenerationTime;
  float                                       TotalFrameTime;
  int                                         FrameResourcesCount;

  vk::UniqueRenderPass                        RenderPass;
  vk::UniqueRenderPass                        PostRenderPass;
  vulkan::DescriptorSetParameters                     DescriptorSet;
  vulkan::ImageParameters                             BackgroundTexture;
  vulkan::ImageParameters                             Texture;
  vk::UniquePipelineLayout                    PipelineLayout;
  vk::UniquePipeline                          GraphicsPipeline;
  vulkan::BufferParameters                            VertexBuffer;
  vulkan::BufferParameters                            InstanceBuffer;
};
