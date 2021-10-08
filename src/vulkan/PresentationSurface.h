#pragma once

#include <vulkan/vulkan.hpp>

namespace vulkan {

class PresentationSurface
{
  vk::UniqueSurfaceKHR m_surface;
  vk::Queue m_graphics_queue;
  vk::Queue m_presentation_queue;

 public:
  PresentationSurface() = default;
  void operator=(vk::UniqueSurfaceKHR&& surface) { m_surface = std::move(surface); }

  void set_queues(vk::Queue graphics_queue, vk::Queue presentation_queue)
  {
    // Only call set_queues once.
    ASSERT(!m_graphics_queue && !m_presentation_queue);
    m_graphics_queue = graphics_queue;
    m_presentation_queue = presentation_queue;
  }

  vk::SurfaceKHR vh_surface() const
  {
    // Only call this function when m_surface is known to be valid.
    ASSERT(m_surface);
    return *m_surface;
  }

  vk::Queue vh_graphics_queue() const
  {
    // First call set_queues.
    ASSERT(m_graphics_queue);
    return m_graphics_queue;
  }

  vk::Queue vh_presentation_queue() const
  {
    // First call set_queues.
    ASSERT(m_presentation_queue);
    return m_presentation_queue;
  }
};

} // namespace vulkan
