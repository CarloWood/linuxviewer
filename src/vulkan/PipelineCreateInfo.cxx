#include "sys.h"
#include "PipelineCreateInfo.h"

namespace vulkan {

#ifdef CWDEBUG
void PipelineCreateInfo::print_on(std::ostream& os) const
{
  os << '{';
  os << '}';
}
#endif

} // namespace vulkan
