#pragma once

#include "Element.h"
#include "Score.h"
#include "utils/Array.h"
#include "utils/reverse_bits.h"
#include <iosfwd>
#include <cassert>
#include <concepts>

namespace vulkan::pipeline::partitions {

class Set;
using SetIndex = utils::ArrayIndex<Set>;

class PartitionTask;

class Set
{
 private:
  elements_t m_elements{};

 public:
  Set() = default;
  Set(Element element) : m_elements(element) { }
  explicit Set(elements_t elements) : m_elements(elements) { }

  utils::bitset::const_iterator<elements_t::mask_type> begin() const { return m_elements.begin(); }
  utils::bitset::const_iterator<elements_t::mask_type> end() const { return m_elements.end(); }

  bool empty() const
  {
    return m_elements.none();
  }

  void clear()
  {
    m_elements.reset();
  }

  Set& add(Set elements)
  {
    // Don't add existing elements.
    assert(!m_elements.test(elements.m_elements));
    m_elements |= elements.m_elements;
    return *this;
  }

  Set& remove(Set set)
  {
    // Don't try to remove non-existing elements.
    assert((m_elements & set.m_elements) == set.m_elements);
    m_elements &= ~set.m_elements;
    return *this;
  }

  Set& toggle(Element element)
  {
    m_elements ^= element;
    return *this;
  }

  bool test(Element element) const
  {
    return m_elements.test(element);
  }

  bool is_single_bit() const
  {
    return m_elements.is_single_bit();
  }

  int element_count() const
  {
    return m_elements.count();
  }

  Score score(PartitionTask const& partition_task) const;

  friend bool operator<(Set lhs, Set rhs)
  {
    return utils::reverse_bits(lhs.m_elements()) < utils::reverse_bits(rhs.m_elements());
  }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

template<typename T>
concept ConceptSet = std::is_convertible_v<T, Set>;

} // namespace vulkan::pipeline::partitions
