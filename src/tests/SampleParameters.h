#pragma once

struct SampleParameters
{
  static constexpr int s_max_object_count = 3000;
  static constexpr int s_quad_tessellation = 300;

  int ObjectCount;
  int PreSubmitCpuWorkTime;
  int PostSubmitCpuWorkTime;
  int SwapchainCount;
  int FrameResourcesCount;
  float m_frame_generation_time;
  float m_total_frame_time;
  bool m_show_fps = true;

  SampleParameters() :
    ObjectCount(889),
    PreSubmitCpuWorkTime(4),
    PostSubmitCpuWorkTime(4),
    SwapchainCount(3),
    FrameResourcesCount(2),
    m_frame_generation_time(0),
    m_total_frame_time(0)
  {
  }
};
