#pragma once

#include "queues/Queue.h"
#include "debug.h"
#include <vulkan/vulkan.hpp>
#include <array>
#ifdef TRACY_ENABLE
#include <TracyVulkan.hpp>
#include "FrameResourceIndex.h"
#endif

namespace task {
class SynchronousWindow;
} // namespace task

namespace vulkan {

#ifdef CWDEBUG
class AmbifixOwner;
#endif

class PresentationSurface
{
  vk::UniqueSurfaceKHR m_surface;
  Queue m_graphics_queue;
  Queue m_presentation_queue;
#ifdef TRACY_ENABLE
  TracyVkCtx m_tracy_context;           // Tracy queue contex for profiling GPU 'zones'.
#endif

 public:
  PresentationSurface() = default;
#ifdef TRACY_ENABLE
  ~PresentationSurface();
#endif
  void operator=(vk::UniqueSurfaceKHR&& surface) { m_surface = std::move(surface); }

  void set_queues(Queue graphics_queue, Queue presentation_queue
#ifdef TRACY_ENABLE
    , task::SynchronousWindow const* owning_window
#endif
      COMMA_CWDEBUG_ONLY(AmbifixOwner const& ambifix));

  vk::SurfaceKHR vh_surface() const
  {
    // Only call this function when m_surface is known to be valid.
    ASSERT(m_surface);
    return *m_surface;
  }

  Queue const& graphics_queue() const
  {
    // First call set_queues.
    ASSERT(m_graphics_queue);
    return m_graphics_queue;
  }

  vk::Queue vh_graphics_queue() const
  {
    // First call set_queues.
    ASSERT(m_graphics_queue);
    return m_graphics_queue;
  }

  Queue presentation_queue() const
  {
    // First call set_queues.
    ASSERT(m_presentation_queue);
    return m_presentation_queue;
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

#ifdef TRACY_ENABLE
  TracyVkCtx tracy_context() const
  {
    return m_tracy_context;
  }
#endif

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan
