#include "sys.h"

#include "PresentationSurface.h"
#include "debug/DebugSetName.h"
#ifdef CWDEBUG
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
    COMMA_CWDEBUG_ONLY(AmbifixOwner const& ambifix))
{
  // Only call set_queues once.
  ASSERT(!m_graphics_queue && !m_presentation_queue);
  m_graphics_queue = graphics_queue;
  DebugSetName(static_cast<vk::Queue>(m_graphics_queue), ambifix(".m_graphics_queue"));
  m_presentation_queue = presentation_queue;
  DebugSetName(static_cast<vk::Queue>(m_presentation_queue), ambifix(".m_presentation_queue"));
}

} // namespace vulkan
