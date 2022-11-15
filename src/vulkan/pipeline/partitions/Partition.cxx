#include "sys.h"
#include "Partition.h"
#include "PartitionIteratorSingleElement.h"
#include "PartitionIteratorWholeSet.h"
#include "PartitionIteratorBruteForce.h"
#include "PartitionIteratorExplode.h"
#include "PartitionTask.h"
#include <cassert>
#include <iostream>
#include <iomanip>

namespace vulkan::pipeline::partitions {

Partition::Partition(PartitionTask const& partition_task) :
  m_sets(partition_task.max_number_of_sets()), m_number_of_elements(partition_task.number_of_elements())
{
  for (auto i = m_sets.ibegin(); i != m_sets.iend(); ++i)
    m_sets[i].clear();

  for (ElementIndexPOD element_index{0}; element_index.m_index < m_number_of_elements; ++element_index.m_index)
  {
    SetIndex set_index{element_index.m_index % partition_task.max_number_of_sets()};
    m_sets[set_index].add(Set(element_index));
  }

  sort();
}

Partition::Partition(PartitionTask const& partition_task, Set set) :
  m_sets(partition_task.max_number_of_sets()), m_number_of_elements(partition_task.number_of_elements())
{
  auto set_iter = m_sets.begin();
  *set_iter = set;
  while (++set_iter != m_sets.end())
    set_iter->clear();
}

Partition::Partition(PartitionTask const& partition_task, utils::Array<Set, max_number_of_elements, SetIndex> const& sets) :
  m_sets(sets.begin(), sets.begin() + partition_task.max_number_of_sets()), m_number_of_elements(partition_task.number_of_elements())
{
  sort();
}

PartitionIterator Partition::end() const
{
  return {};
}

#ifdef CWDEBUG
void Partition::print_sets() const
{
  char const* separator = "";
  for (ElementIndex e = element_ibegin(); e != element_iend(); ++e)
  {
    for (SetIndex g = m_sets.ibegin(); g != m_sets.iend(); ++g)
    {
      if (m_sets[g].test(e))
      {
        std::cout << separator << Element{e} << "=" << g.get_value();
        separator = ", ";
      }
    }
  }
}

void Partition::print_on(std::ostream& os) const
{
  os << '<';
  char const* prefix = "";
  for (SetIndex g = m_sets.ibegin(); g != m_sets.iend(); ++g)
  {
    if (m_sets[g].empty())
      break;
    os << prefix << m_sets[g];
    prefix = ", ";
  }
  os << '>';
}
#endif

SetIndex Partition::number_of_sets() const
{
  SetIndex g = m_sets.ibegin();
  while (g != m_sets.iend())
  {
    if (m_sets[g].empty())
      break;
    ++g;
  }
  return g;
}

bool Partition::is_alone(Element element) const
{
  for (SetIndex g = m_sets.ibegin(); g != m_sets.iend(); ++g)
    if (m_sets[g].test(element))
      return m_sets[g].is_single_bit();
  // Never reached.
  assert(false);
  return false;
}

SetIndex Partition::set_of(Element element) const
{
  for (SetIndex g = m_sets.ibegin(); g != m_sets.iend(); ++g)
    if (m_sets[g].test(element))
      return g;
  // Never reached.
  assert(false);
  return {};
}

SetIndex Partition::first_empty_set() const
{
  for (SetIndex g = m_sets.ibegin(); g != m_sets.iend(); ++g)
    if (m_sets[g].empty())
      return g;
  return m_sets.iend();
}

Score Partition::score(PartitionTask const& partition_task) const
{
  Score sum;
  for (SetIndex g = m_sets.ibegin(); g != m_sets.iend(); ++g)
  {
    if (m_sets[g].empty())
      break;
    sum += m_sets[g].score(partition_task);
  }
  return sum;
}

PartitionIteratorExplode Partition::sbegin(PartitionTask const& partition_task)
{
  return {partition_task, *this};
}

PartitionIteratorExplode Partition::send()
{
  return {};
}

Score Partition::find_local_maximum(PartitionTask const& partition_task)
{
//  DoutEntering(dc::notice, "Partition::find_local_maximum(" << partition_task << ")");
  Score current_score = score(partition_task);
  constexpr int number_of_algorithms = 2;
  int no_improvement_count = 0;
  do
  {
    for (int algorithm = 0; algorithm < number_of_algorithms; ++algorithm)
    {
//      Dout(dc::notice, "algorithm = " << algorithm);
      Score last_score;

      // Anticipate that the current algorithm will fail to find any improvement.
      ++no_improvement_count;
      // Run algorithm 'algorithm'.
      do
      {
//        Dout(dc::notice, *this << " = " << current_score);
        last_score = current_score;

        for (auto neighboring_partition_iterator =
            algorithm == 0 ? begin<PartitionIteratorWholeSet>() : begin<PartitionIteratorSingleElement>() ;
             neighboring_partition_iterator != end();
             ++neighboring_partition_iterator)
        {
//          Dout(dc::notice, "neighboring_partition_iterator = " << neighboring_partition_iterator);
          Partition neighboring_partition = *neighboring_partition_iterator;
//          Dout(dc::notice, "neighboring_partition = " << neighboring_partition);
          Score neighboring_score = neighboring_partition.score(partition_task);
//          Dout(dc::notice, "neighboring_score = " << neighboring_score);
          if (neighboring_score > current_score)
          {
            m_sets = neighboring_partition.m_sets;
            current_score = neighboring_score;
            no_improvement_count = 1;
          }
        }
      }
      while (current_score > last_score);       // If 'algorithm' found an improvement, run the same algorithm again.

      // If none of the algorithms could improve, terminate the program.
      if (no_improvement_count == number_of_algorithms)
        break;
    }
  }
  while (no_improvement_count < number_of_algorithms);
  return current_score;
}

void Partition::reduce_sets(PartitionTask const& partition_task)
{
//  DoutEntering(dc::notice, "Partition::reduce_sets(" << partition_task << ") [" << *this << "]");
  find_local_maximum(partition_task);
  // <{A}, {B}, {C}, {}, {}>
  //                 ^
  //                 |
  //              fes=3
  int8_t fes = first_empty_set().get_value();
  for (int i = 0; i < fes - partition_task.max_number_of_sets(); ++i)
  {
    Score max_score{negative_inf};
    for (PartitionIterator iter = begin<PartitionIteratorWholeSet>(); iter != end(); ++iter)
    {
      Partition merged_partition = *iter;    // Has one set less.
      Score merged_score = merged_partition.score(partition_task);
      if (merged_score > max_score)
      {
        max_score = merged_score;
        *this = merged_partition;
      }
    }
  }
  ASSERT(first_empty_set().get_value() <= partition_task.max_number_of_sets());
  m_sets.resize(partition_task.max_number_of_sets());
}

} // namespace vulkan::pipeline::partitions
