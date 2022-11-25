#include "sys.h"
#include "DescriptorUpdateInfo.h"
#include "FrameResourceCapableDescriptorSet.h"
#include "debug.h"

namespace vulkan::descriptor {

#ifdef CWDEBUG
void DescriptorUpdateInfo::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_owning_window:" << m_owning_window <<
      ", m_factory_characteristic_id:" << m_factory_characteristic_id <<
      ", m_fill_index:" << m_fill_index <<
      ", m_array_size:" << m_array_size <<
      ", m_descriptor_set:" << m_descriptor_set <<
      ", m_binding:" << m_binding <<
      ", m_has_frame_resource:" << m_has_frame_resource;
  os << '}';
}
#endif

} // namespace vulkan::descriptor
