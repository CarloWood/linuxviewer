#pragma once

#include "PartitionIteratorBase.h"

namespace vulkan::pipeline::partitions {

class PartitionIteratorWholeSet final : public PartitionIteratorBase
{
 private:
  // Second loop variable.
  SetIndex const m_first_empty_set;

 public:
  // Create an end iterator.
  PartitionIteratorWholeSet() = default;

  // Create a begin iterator.
  PartitionIteratorWholeSet(Partition const& orig) : PartitionIteratorBase(orig, SetIndex{0}), m_first_empty_set(orig.first_empty_set())
  {
  }

 private:
  Set moved_elements() const override
  {
    return m_original_partition.set(m_from_set);
  }

  void increment() override;

  bool unequal(PartitionIteratorBase const& rhs) const override
  {
    return m_from_set != rhs.from_set() || m_to_set != rhs.to_set();
  }
};

} // namespace vulkan::pipeline::partitions
