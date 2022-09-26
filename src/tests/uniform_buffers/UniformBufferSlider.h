#pragma once

#include "shader_builder/shader_resource/UniformBuffer.h"
#include <atomic>

template<typename ENTRY>
class UniformBufferSlider : public vulkan::shader_builder::shader_resource::UniformBuffer<ENTRY>
{
 private:
  std::atomic_bool m_ready{0};

 public:
  using vulkan::shader_builder::shader_resource::UniformBuffer<ENTRY>::UniformBuffer;

  void ready() override
  {
    m_ready.store(true, std::memory_order::release);
  }

  bool is_ready() const
  {
    return m_ready.load(std::memory_order::acquire);
  }
};
