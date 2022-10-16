#include "sys.h"

#include "PresentationSurface.h"
#include "SynchronousWindow.h"
#ifdef CWDEBUG
#include "debug/DebugSetName.h"
#include "debug/debug_ostream_operators.h"
#endif

namespace vulkan {

#ifdef CWDEBUG
void PresentationSurface::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_surface:" << *m_surface << ", ";
  os << "m_graphics_queue:" << m_graphics_queue << ", ";
  os << "m_presentation_queue:" << m_presentation_queue;
  os << '}';
}
#endif

void PresentationSurface::set_queues(Queue graphics_queue, Queue presentation_queue
#ifdef TRACY_ENABLE
    , task::SynchronousWindow const* owning_window
#endif
    COMMA_CWDEBUG_ONLY(AmbifixOwner const& ambifix))
{
  // Only call set_queues once.
  ASSERT(!m_graphics_queue && !m_presentation_queue);
  m_graphics_queue = graphics_queue;
  DebugSetName(static_cast<vk::Queue>(m_graphics_queue), ".m_graphics_queue" + ambifix);
  m_presentation_queue = presentation_queue;
  DebugSetName(static_cast<vk::Queue>(m_presentation_queue), ".m_presentation_queue" + ambifix);
#ifdef TRACY_ENABLE
  m_tracy_context = owning_window->logical_device()->tracy_context(m_graphics_queue
      COMMA_CWDEBUG_ONLY(".m_tracy_context" + ambifix));
#endif
}

#ifdef TRACY_ENABLE
PresentationSurface::~PresentationSurface()
{
  TracyVkDestroy(m_tracy_context);
}
#endif

} // namespace vulkan
