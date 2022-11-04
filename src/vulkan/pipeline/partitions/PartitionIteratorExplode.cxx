#include "sys.h"
#include "PartitionIteratorExplode.h"
#include "utils/VectorCompare.h"

namespace vulkan::pipeline::partitions {

struct SetIteratorCompare
{
  bool operator()(PartitionIteratorExplode::SetIterator const& lhs, PartitionIteratorExplode::SetIterator const& rhs) const
  {
    return lhs.m_pair_triplet_iterator.score_difference() < rhs.m_pair_triplet_iterator.score_difference();
  }
};

PartitionIteratorExplode::PartitionIteratorExplode(PartitionTask const& partition_task, Partition orig) : m_orig(orig)
{
//  DoutEntering(dc::notice, "PartitionIteratorExplode::PartitionIteratorExplode(" << orig << ")");
  for (SetIndex set_index = m_orig.set_ibegin(); set_index != m_orig.set_iend(); ++set_index)
  {
    Set set = m_orig.set(set_index);
    int element_count = set.element_count();
    if (element_count == 0)
      break;
    if (element_count > 2)
      m_pair_triplet_iterators.emplace_back(partition_task, set, set_index, 1);
  }
  // Start m_pair_triplet_iterators.size() nested for loops, all beginning at 0.
  m_pair_triplet_counters.initialize(m_pair_triplet_iterators.size(), 0);
  if (m_pair_triplet_iterators.empty())
    return;
  std::sort(m_pair_triplet_iterators.begin(), m_pair_triplet_iterators.end(), SetIteratorCompare{});
  while (increase_loop_count())
    ;
  for (SetIterator& set_iterator : m_pair_triplet_iterators)
    set_iterator.reset();
//  Dout(dc::notice, "m_pair_triplet_iterators = " << m_pair_triplet_iterators);
  // Start MultiLoop code.
  while (m_pair_triplet_counters() < m_pair_triplet_iterators[*m_pair_triplet_counters].m_loop_count)
  {
    if (m_pair_triplet_counters.inner_loop())
      return;
    m_pair_triplet_counters.start_next_loop_at(0);
  }
}

PartitionIteratorExplode& PartitionIteratorExplode::operator++()
{
  // for (int l0 = 0; l0 < m_pair_triplet_iterators[0].m_loop_count; ++l0)
  // {
  //   for (int l1 = 0; l1 < m_pair_triplet_iterators[1].m_loop_count; ++l1)
  //   {
  //     for (int l2 = 0; l2 < m_pair_triplet_iterators[2].m_loop_count; ++l2)
  //     {
  //       ... inner loop ...
  //       ++(m_pair_triplet_iterators[2].m_pair_triplet_iterator);
  //     }
  //     m_pair_triplet_iterators[2].m_pair_triplet_iterator.reset();
  //     ++(m_pair_triplet_iterators[1].m_pair_triplet_iterator);
  //   }
  //   m_pair_triplet_iterators[1].m_pair_triplet_iterator.reset();
  //   ++(m_pair_triplet_iterators[0].m_pair_triplet_iterator);
  // }
  //
  ++(m_pair_triplet_iterators.rbegin()->m_pair_triplet_iterator);
  m_pair_triplet_counters.start_next_loop_at(0);
  do
  {
    while (m_pair_triplet_counters() < m_pair_triplet_iterators[*m_pair_triplet_counters].m_loop_count)
    {
      if (m_pair_triplet_counters.inner_loop())             // Most inner loop.
        return *this;
      m_pair_triplet_counters.start_next_loop_at(0);
    }
    if (int loop = m_pair_triplet_counters.end_of_loop(); loop >= 0)
    {
      // End of loop code for loop 'loop'.
      ASSERT(loop + 1 < m_pair_triplet_iterators.size());
      m_pair_triplet_iterators[loop + 1].m_pair_triplet_iterator.reset();
      ++(m_pair_triplet_iterators[loop].m_pair_triplet_iterator);
    }
    m_pair_triplet_counters.next_loop();
  }
  while (!m_pair_triplet_counters.finished());
  return *this;
}

bool PartitionIteratorExplode::increase_loop_count()
{
  //    |       |       |
  //    v       v       v
  //  104(15)  90(25)  68(0)
  //   89(inf) 65(25)  68(13)
  //           40(inf) 55(inf)

  // 104, 90, 68
  // 104, 90, 68
  // 104, 90, 55
  //  89, 90, 55
  //  89, 65, 55
  //  89, 40, 55

  PairTripletIteratorExplode const& pair_triplet_iterator = ++(m_pair_triplet_iterators.begin()->m_pair_triplet_iterator);
  if (pair_triplet_iterator.is_end())
    return false;       // Reached the end.

  ++(m_pair_triplet_iterators.begin()->m_loop_count);
  unsigned long total_loop_count = 1;
  for (SetIterator const& set_iterator : m_pair_triplet_iterators)
    total_loop_count *= set_iterator.m_loop_count;

  if (total_loop_count >= total_loop_count_limit)
    return false;

  Score new_score_difference = pair_triplet_iterator.score_difference();
  auto end = std::find_if(m_pair_triplet_iterators.begin() + 1, m_pair_triplet_iterators.end(),
      [&](SetIterator const& v){ return v.m_pair_triplet_iterator.score_difference() >= new_score_difference; });
  if (m_pair_triplet_iterators.begin() + 1 != end)
    std::rotate(m_pair_triplet_iterators.begin(), m_pair_triplet_iterators.begin() + 1, end);

  return true;
}

Partition PartitionIteratorExplode::get_partition(PartitionTask const& partition_task) const
{
  Partition result(m_orig, m_orig.element_iend()());
  SetIndex first_empty_set = result.first_empty_set();
  for (auto& set_iterator : m_pair_triplet_iterators)
  {
    PairTripletIteratorExplode const& pair_triplet_iterator = set_iterator.m_pair_triplet_iterator;
    Set pair_triplet = *pair_triplet_iterator;
    SetIndex set_index = set_iterator.m_set_index;
    result.remove_from(set_index, pair_triplet);
    result.add_to(first_empty_set++, pair_triplet);
  }
  result.sort();
  result.reduce_sets(partition_task);
  return result;
}

void PartitionIteratorExplode::SetIterator::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_pair_triplet_iterator:" << m_pair_triplet_iterator <<
      ", m_set_index:" << m_set_index <<
      ", m_loop_count:" << m_loop_count;
  os << '}';
}

void PartitionIteratorExplode::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_orig:" << m_orig <<
      ", m_pair_triplet_iterators:" << m_pair_triplet_iterators;
  os << '}';
}

} // namespace vulkan::pipeline::partitions
