#include "sys.h"
#include "Set.h"
#include "ElementPair.h"
#include "PartitionTask.h"
#include <iostream>
#include <vector>

namespace vulkan::pipeline::partitions {

Score Set::score(PartitionTask const& partition_task) const
{
  Score sum;
  if (m_elements.any() && !m_elements.is_single_bit())
  {
    for (auto bit_iter1 = m_elements.begin(); bit_iter1 != m_elements.end(); ++bit_iter1)
    {
      auto bit_iter2 = bit_iter1;
      ++bit_iter2;
      while (bit_iter2 != m_elements.end())
      {
        int score_index = ElementPair{elements_t::mask2index((*bit_iter1)()), elements_t::mask2index((*bit_iter2)())}.score_index();
        sum += partition_task.score(score_index);
        ++bit_iter2;
      }
    }
  }
  return sum;
}

#ifdef CWDEBUG
void Set::print_on(std::ostream& os) const
{
  os << '{';
  ElementIndex const last_element_index = m_elements.mssbi();
  ElementIndex element_index = m_elements.lssbi();
  while (element_index <= last_element_index)
  {
    Element element{element_index};
    if (m_elements.test(element))
      os << element;
    ++element_index;
  }
  os << '}';
}
#endif

} // namespace vulkan::pipeline::partitions
