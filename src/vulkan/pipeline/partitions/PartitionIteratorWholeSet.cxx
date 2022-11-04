#include "sys.h"
#include "PartitionIteratorWholeSet.h"

namespace vulkan::pipeline::partitions {

void PartitionIteratorWholeSet::increment()
{
  do
  {
    // Increment the inner loop variable until it points to the first empty set, which might be one beyond the end of the vector.
    if (++m_from_set == m_first_empty_set)
    {
      // Increment the outer loop variable.
      if (++m_to_set == m_first_empty_set)
      {
        // We reached the end.
        set_to_end();
        break;
      }
      // For this algorithm it doesn't make sense to have m_from_set < m_to_set.
      // Start m_from_set at m_to_set + 1, if that is possible.
      m_from_set = m_to_set;
    }
  }
  while (m_to_set == m_from_set);
}

} // namespace vulkan::pipeline::partitions
