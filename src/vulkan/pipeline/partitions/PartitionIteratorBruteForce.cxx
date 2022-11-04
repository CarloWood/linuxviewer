#include "sys.h"
#include "PartitionIteratorBruteForce.h"
#include "PartitionTask.h"

namespace vulkan::pipeline::partitions {

PartitionIteratorBruteForce::PartitionIteratorBruteForce(PartitionTask const& partition_task) :
  m_multiloop(partition_task.number_of_elements()), m_loop_value_count(partition_task.max_number_of_sets()), m_partition_task(&partition_task)
{
  for (; !m_multiloop.finished(); m_multiloop.next_loop())
  {
    for (;;)
    {
      // A loop ends when moving the element to the next set would leave its current set empty.
      if (m_multiloop() > 0 && --m_loop_value_count[m_multiloop() - 1] == 0)
        break;
      // We also can't go beyond the maximum number of allowed sets.
      if (m_multiloop() == m_loop_value_count.size())
        break;
      ++m_loop_value_count[m_multiloop()];

      if (m_multiloop.inner_loop())             // Most inner loop.
      {
        // We're ready to execute operator*/operator++ while in the inner loop.
        return;
      }
      m_multiloop.start_next_loop_at(0);
    }
  }
}

PartitionIteratorBruteForce& PartitionIteratorBruteForce::operator++()
{
  m_multiloop.start_next_loop_at(0);
  do
  {
    for (;;)
    {
      if (m_multiloop() > 0 && --m_loop_value_count[m_multiloop() - 1] == 0)
        break;
      if (m_multiloop() == m_loop_value_count.size())
        break;
      ++m_loop_value_count[m_multiloop()];
      if (m_multiloop.inner_loop())             // Most inner loop.
        return *this;
      m_multiloop.start_next_loop_at(0);
    }
    m_multiloop.next_loop();
  }
  while (!m_multiloop.finished());
  return *this;
}

Partition PartitionIteratorBruteForce::operator*() const
{
  utils::Array<Set, max_number_of_elements, SetIndex> sets;
  for (SetIndex g = sets.ibegin(); g != sets.iend(); ++g)
    sets[g].clear();
  ElementIndex const number_of_elements{ElementIndexPOD{m_partition_task->number_of_elements()}};
  for (ElementIndex l{utils::bitset::index_begin}; l < number_of_elements; ++l)
    sets[SetIndex{m_multiloop[l()]}].add(Element{l});
  return {*m_partition_task, sets};
}

} // namespace vulkan::pipeline::partitions
