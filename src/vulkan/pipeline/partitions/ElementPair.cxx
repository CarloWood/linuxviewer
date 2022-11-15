#include "sys.h"
#include "ElementPair.h"
#include "PartitionTask.h"
#include <iostream>

namespace vulkan::pipeline::partitions {

Score const& ElementPair::score(PartitionTask const& partition_task) const
{
  return partition_task.score(score_index());
}

#ifdef CWDEBUG
void ElementPair::print_on(std::ostream& os) const
{
  os << '(' << m_element1 << ", " << m_element2 << ')';
}
#endif

} // namespace vulkan::pipeline::partitions
