#ifndef PARTITION_ITERATOR_BASE
#define PARTITION_ITERATOR_BASE

#include "Partition.h"
#include "utils/Badge.h"

namespace vulkan::pipeline::partitions {

class PartitionIteratorBase
{
 protected:
  // The partition that we iterator over.
  Partition const m_original_partition;
  SetIndex const m_number_of_sets_original;

  SetIndex m_from_set{};            // The set that we're moving from.
  // Loop variable.
  SetIndex m_to_set{0};             // The set that we're moving to.

 public:
  // Construct the 'end' iterator.
  PartitionIteratorBase() : m_to_set{} { }

  PartitionIteratorBase(Partition const& orig, SetIndex from_set_index) :
    m_original_partition(orig),
    m_number_of_sets_original(orig.number_of_sets()),
    m_from_set(from_set_index)
  {
//  DoutEntering(dc::notice, "PartitionIteratorBase::PartitionIteratorBase(" << orig << ", " << from_set_index << ")");
    ASSERT(!m_from_set.undefined());
  }

  virtual ~PartitionIteratorBase() { }

  Partition const& original_partition() const
  {
    return m_original_partition;
  }

  ElementIndex element_ibegin() const { return m_original_partition.element_ibegin(); }
  ElementIndex element_iend() const { return m_original_partition.element_iend(); }

  SetIndex from_set() const
  {
    return m_from_set;
  }

  SetIndex to_set() const
  {
    return m_to_set;
  }

  void kick_start(utils::Badge<PartitionIterator>)
  {
    if (m_from_set == m_to_set)
      increment();
  }

  void set_to_end()
  {
    m_from_set.set_to_undefined();
  }

  bool is_end() const
  {
    return m_from_set.undefined();
  }

  virtual void increment() = 0;
  virtual Set moved_elements() const = 0;
  virtual bool unequal(PartitionIteratorBase const& rhs) const = 0;

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::pipeline::partitions

#endif // PARTITION_ITERATOR_BASE
