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

  SampleParameters() :
  ObjectsCount(100),
  PreSubmitCpuWorkTime(0),
  PostSubmitCpuWorkTime(0),
  FrameGenerationTime(0),
  TotalFrameTime(0),
  FrameResourcesCount(1),
  RenderPass(),
  PostRenderPass(),
  DescriptorSet(),
  BackgroundTexture(),
  Texture(),
  PipelineLayout(),
  GraphicsPipeline(),
  VertexBuffer(),
  InstanceBuffer()
  {
  }
};
