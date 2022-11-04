#pragma once

#include "Element.h"
#include "Score.h"
#include "debug.h"

namespace vulkan::pipeline::partitions {

class PartitionTask;

class ElementPair
{
 private:
  ElementIndex m_element1;
  ElementIndex m_element2;

 public:
  // Construct an undefined element pair.
  ElementPair() = default;
  ElementPair(ElementIndex element1, ElementIndex element2) : m_element1(element1), m_element2(element2)
  {
    ASSERT(element1 != element2);
  }

  Score const& score(PartitionTask const& partition_task) const;

  int score_index() const
  {
    return 64 * m_element1() + m_element2();
  }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::pipeline::partitions
