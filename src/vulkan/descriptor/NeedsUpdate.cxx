#include "sys.h"
#include "NeedsUpdate.h"
#include "FrameResourceCapableDescriptorSet.h"
#include "debug.h"

namespace vulkan::descriptor {

#ifdef CWDEBUG
void NeedsUpdate::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_owning_window:" << m_owning_window <<
      ", m_descriptor_set:" << m_descriptor_set <<
      ", m_binding:" << m_binding <<
      ", m_has_frame_resource:" << m_has_frame_resource;
  os << '}';
}
#endif

} // namespace vulkan::descriptor
