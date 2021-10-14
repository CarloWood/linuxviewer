#pragma once

#include "Queue.h"
#include "debug.h"
#include <vulkan/vulkan.hpp>
#include <array>

namespace vulkan {

class PresentationSurface
{
  vk::UniqueSurfaceKHR m_surface;
  Queue m_graphics_queue;
  Queue m_presentation_queue;

 public:
  PresentationSurface() = default;
  void operator=(vk::UniqueSurfaceKHR&& surface) { m_surface = std::move(surface); }

  void set_queues(Queue graphics_queue, Queue presentation_queue)
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

  // Return true if the graphics- and the presentation queue family is the same family, false otherwise.
  bool uses_multiple_queue_families() const
  {
    return m_graphics_queue.queue_family() != m_presentation_queue.queue_family();
  }

  std::array<uint32_t, 2> queue_family_indices() const
  {
    return { static_cast<uint32_t>(m_graphics_queue.queue_family().get_value()),
             static_cast<uint32_t>(m_presentation_queue.queue_family().get_value()) };
  }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan
