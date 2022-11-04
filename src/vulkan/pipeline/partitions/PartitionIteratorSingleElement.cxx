#include "sys.h"
#include "PartitionIteratorSingleElement.h"

namespace vulkan::pipeline::partitions {

void PartitionIteratorSingleElement::increment()
{
  do
  {
    if (++m_to_set == (m_current_element_is_alone ? m_number_of_sets_original - 1 : m_number_of_sets_original + 1) ||
        m_to_set == m_original_partition.set_iend())
    {
      m_to_set.set_to_zero();
      if (++m_current_element_index == m_original_partition.element_iend())
      {
        set_to_end();
        break;
      }
      m_current_element_is_alone = m_original_partition.is_alone(m_current_element_index);
      m_from_set = m_original_partition.set_of(m_current_element_index);
    }
  }
  while (m_to_set == m_from_set);
}

} // namespace vulkan::pipeline::partitions
