#include "sys.h"
#include "FrameResourceCapableDescriptorSet.h"
#ifdef CWDEBUG
#include "cwds/debug_ostream_operators.h"
#endif

namespace vulkan::descriptor {

#ifdef CWDEBUG
void FrameResourceCapableDescriptorSet::print_on(std::ostream& os) const
{
  if (m_descriptor_set.size() == 1)
    os << static_cast<vk::DescriptorSet>(*this);
  else
    os << m_descriptor_set;
}
#endif

} // namespace vulkan::descriptor
