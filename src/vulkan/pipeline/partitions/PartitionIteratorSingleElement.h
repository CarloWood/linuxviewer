#pragma once

#include "PartitionIteratorBase.h"

namespace vulkan::pipeline::partitions {

class PartitionIteratorSingleElement final : public PartitionIteratorBase
{
 private:
  // Second loop variable.
  ElementIndex m_current_element_index{utils::bitset::index_begin};      // 0...N-1
  // Cached information about the current element.
  bool m_current_element_is_alone{};            // Whether or not it is the only elment in that set (in the original).

 public:
  // Create an end iterator.
  PartitionIteratorSingleElement() : m_current_element_index{utils::bitset::index_pre_begin} { }

  // Create a begin iterator.
  PartitionIteratorSingleElement(Partition const& orig) :
    PartitionIteratorBase(orig, orig.set_of(utils::bitset::index_begin)),
    m_current_element_is_alone(orig.is_alone(m_current_element_index))
  {
  }

 private:
  Set moved_elements() const override
  {
    return Element{m_current_element_index};
  }

  void increment() override;

  bool unequal(PartitionIteratorBase const& rhs) const override
  {
    return m_current_element_index != static_cast<PartitionIteratorSingleElement const&>(rhs).m_current_element_index || m_to_set != rhs.to_set();
  }
};

} // namespace vulkan::pipeline::partitions
