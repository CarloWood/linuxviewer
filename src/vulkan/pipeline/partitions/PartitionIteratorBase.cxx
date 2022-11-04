#include "sys.h"
#include "PartitionIteratorBase.h"

namespace vulkan::pipeline::partitions {

void PartitionIteratorBase::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_original_partition:" << m_original_partition <<
      ", m_number_of_sets_original:" << m_number_of_sets_original <<
      ", m_from_set:" << m_from_set <<
      ", m_to_set:" << m_to_set;
  os << '}';
}

} // namespace vulkan::pipeline::partitions
