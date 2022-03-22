#include "sys.h"
#include "Handle.h"

namespace vulkan::pipeline {

#ifdef CWDEBUG
void Handle::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_pipeline_factory_index:" << m_pipeline_factory_index <<
      ", m_pipeline_index:" << m_pipeline_index;
  os << '}';
}
#endif

} // namespace vulkan::pipeline
