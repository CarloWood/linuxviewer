#pragma once

#include "Defs.h"
#include "utils/BitSet.h"
#include "utils/has_print_on.h"
#include <iosfwd>

namespace vulkan::pipeline::partitions {
using utils::has_print_on::operator<<;

class Element;
using ElementIndex = elements_t::Index;
using ElementIndexPOD = utils::bitset::IndexPOD;

class Element
{
 private:
  elements_t m_bit;
  ElementIndex m_index;
  char m_name;

 public:
  Element(ElementIndex index) : m_bit(index_to_bit(index)), m_index(index), m_name(index_to_name(index)) { }
  constexpr Element(ElementIndexPOD index) : m_bit(index_to_bit(index)), m_index(index), m_name(index_to_name(index)) { }
  constexpr Element(elements_t bit) : m_bit(bit), m_index(bit.lssbi()), m_name(index_to_name(m_index)) { }
  explicit constexpr Element(char name) : m_bit(index_to_bit(name_to_index(name))), m_index(name_to_index(name)), m_name(name) { }

  constexpr operator elements_t() const { return m_bit; }

  constexpr Element operator|(Element const& rhs) const { return m_bit | rhs; }
  constexpr Element operator&(Element const& rhs) const { return m_bit & rhs; }
  constexpr Element operator^(Element const& rhs) const { return m_bit ^ rhs; }
  constexpr bool operator!=(Element const& rhs) const { return m_bit != rhs; }
  constexpr auto operator<=>(Element const& rhs) const { return m_bit <=> rhs.m_bit; }

 private:
  static constexpr char index_to_name(ElementIndex index)
  {
    return 'A' + index();
  }

  static constexpr elements_t index_to_bit(ElementIndex index)
  {
    return elements_t{1} << index();
  }

  static constexpr ElementIndex name_to_index(char name)
  {
    return ElementIndex{ElementIndexPOD{static_cast<int8_t>(name - 'A')}};
  }

#ifdef CWDEBUG
 public:
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::pipeline::partitions
