#pragma once

#include "FactoryCharacteristicId.h"
#include "ConsecutiveRange.h"

namespace vulkan::pipeline {
using utils::has_print_on::operator<<;

class FactoryCharacteristicKey
{
 private:
  FactoryCharacteristicId m_id;                 // A pipeline factory / characteristic-range pair.
  ConsecutiveRange m_subrange;                  // The (sub)range of m_id that this key refers to.

 public:
  FactoryCharacteristicKey() = default;

  // Construct a FactoryCharacteristicKey for id with a range of 1 at fill_index.
  FactoryCharacteristicKey(FactoryCharacteristicId const& id, int fill_index) :
    m_id(id), m_subrange(fill_index, fill_index + 1) { }

  // Construct a FactoryCharacteristicKey for id with the given range.
  FactoryCharacteristicKey(FactoryCharacteristicId const& id, ConsecutiveRange subrange) :
    m_id(id), m_subrange(subrange) { }

  FactoryCharacteristicKey& operator=(FactoryCharacteristicKey const& rhs)
  {
    m_id = rhs.m_id;
    m_subrange = rhs.m_subrange;
    return *this;
  }

  // Accessors.
  FactoryCharacteristicId const& id() const { return m_id; }
  ConsecutiveRange subrange() const { return m_subrange; }

  void set_subrange(ConsecutiveRange subrange) { m_subrange = subrange; }
  void extend_subrange(int fill_index) { m_subrange.extend_subrange(fill_index); }
  void extend_subrange(ConsecutiveRange subrange) { m_subrange.extend_subrange(subrange); }

  friend bool operator<(FactoryCharacteristicKey const& lhs, FactoryCharacteristicKey const& rhs)
  {
    if (lhs.m_id < rhs.m_id)
      return true;
    else if (rhs.m_id < lhs.m_id)
      return false;
    return lhs.m_subrange < rhs.m_subrange;
  }

  friend bool operator==(FactoryCharacteristicKey const& lhs, FactoryCharacteristicKey const& rhs)
  {
    return lhs.m_id == rhs.m_id && lhs.m_subrange.overlaps(rhs.m_subrange);
  }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::pipeline
