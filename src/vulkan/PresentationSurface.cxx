#include "sys.h"

#ifdef CWDEBUG
#include "PresentationSurface.h"
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

} // namespace vulkan
