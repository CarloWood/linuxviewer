#ifndef PARTITION_H
#define PARTITION_H

#include "Set.h"
#include "utils/Vector.h"
#include "utils/RandomNumber.h"
#include <algorithm>
#include "debug.h"

namespace vulkan::pipeline::partitions {

class PartitionIterator;
class PartitionIteratorBruteForce;
class PartitionIteratorExplode;
class PartitionTask;

class Partition
{
 private:
  friend class PartitionIterator;
  utils::Vector<Set, SetIndex> m_sets;
  int8_t m_number_of_elements{};

 public:
  Partition() = default;
  Partition(PartitionTask const& partition_task);
  Partition(PartitionTask const& partition_task, Set set);
  Partition(Partition const& orig, int8_t max_number_of_sets) : m_sets(orig.m_sets), m_number_of_elements(orig.m_number_of_elements)
  {
    // We only support growing the number of allowed sets.
    ASSERT(max_number_of_sets >= orig.m_sets.size());
    m_sets.resize(max_number_of_sets);
    for (SetIndex set_index = orig.m_sets.iend(); set_index < m_sets.iend(); ++set_index)
      m_sets[set_index].clear();
  }

  // Only used by PartitionIteratorBruteForce.
  Partition(PartitionTask const& partition_task, utils::Array<Set, max_number_of_elements, SetIndex> const& sets);

#ifdef CWDEBUG
  void print_sets() const;
#endif

  Set set(SetIndex set_index) const
  {
    return m_sets[set_index];
  }

  ElementIndex element_ibegin() const { return {utils::bitset::index_begin}; }
  ElementIndex element_iend() const { return ElementIndexPOD{m_number_of_elements}; }

  SetIndex set_ibegin() const { return m_sets.ibegin(); }
  SetIndex set_iend() const { return m_sets.iend(); }

  template<typename Algorithm>
  PartitionIterator begin() const;

  PartitionIterator end() const;

  PartitionIteratorExplode sbegin(PartitionTask const& partition_task);
  PartitionIteratorExplode send();

  SetIndex number_of_sets() const;
  bool is_alone(Element element) const;
  SetIndex set_of(Element element) const;
  SetIndex first_empty_set() const;
  Score score(PartitionTask const& partition_task) const;

  void sort()
  {
    std::sort(m_sets.rbegin(), m_sets.rend());
  }

  void add_to(SetIndex set_index, Set set)
  {
    ASSERT(set_index < m_sets.iend());
    m_sets[set_index].add(set);
  }

  void remove_from(SetIndex set_index, Set set)
  {
    m_sets[set_index].remove(set);
  }

  Score find_local_maximum(PartitionTask const& partition_task);
  void reduce_sets(PartitionTask const& partition_task);

  friend bool operator<(Partition const& lhs, Partition const& rhs)
  {
    return lhs.m_sets < rhs.m_sets;
  }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::pipeline::partitions

#endif // PARTITION_H

#ifndef PARTITION_ITERATOR_H
#include "PartitionIterator.h"
#endif

#ifndef PARTITION_H_definitions
#define PARTITION_H_definitions

namespace vulkan::pipeline::partitions {

template<typename Algorithm>
PartitionIterator Partition::begin() const
{
  return {*this, std::make_unique<Algorithm>(*this)};
}

} // namespace vulkan::pipeline::partitions

#endif // PARTITION_H_definitions
