#include "sys.h"
#include "CacheData.h"
#include "PipelineCache.h"

namespace vulkan::pipeline {

#ifdef CWDEBUG
void CacheData::print_on(std::ostream& os) const
{
  os << '{';
  os << "logical_device:" << m_logical_device <<
      ", owning_factory:" << m_owning_factory;
  os << '}';
}
#endif

} // namespace vulkan::pipeline
