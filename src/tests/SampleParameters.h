#pragma once

struct SampleParameters
{
  static constexpr int s_max_object_count = 1000;
  static constexpr int s_quad_tessellation = 40;

  int ObjectCount;
  int PreSubmitCpuWorkTime;
  int PostSubmitCpuWorkTime;
  int FrameResourcesCount;
  float m_frame_generation_time;
  float m_total_frame_time;
  bool m_show_fps = true;

  SampleParameters() :
    ObjectCount(100),
    PreSubmitCpuWorkTime(0),
    PostSubmitCpuWorkTime(0),
    FrameResourcesCount(1),
    m_frame_generation_time(0),
    m_total_frame_time(0)
  {
  }
};
