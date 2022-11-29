#pragma once

#include <cstdlib>
#include "debug.h"

namespace vulkan::descriptor {

// An array range [ibegin, |iend|>.
class ArrayElementRange
{
 private:
  uint32_t m_ibegin;    // The first element.
  int32_t m_iend;       // Can be negative if refering to an unbounded descriptor array.

 public:
  // Construct an uninitialized ArrayElementRange.
  ArrayElementRange() = default;

  // Construct an ArrayElementRange from ibegin till iend.
  ArrayElementRange(uint32_t ibegin, int32_t iend) : m_ibegin(ibegin), m_iend(iend)
  {
    // iend must be larger (or equal for an empty range) than ibegin.
    ASSERT(ibegin <= std::abs(iend));
  }

  // Accessors.
  uint32_t ibegin() const { return m_ibegin; }
  uint32_t iend() const { return static_cast<uint32_t>(std::abs(m_iend)); }
  bool refers_to_unbounded_array() const { return m_iend < 0; }
  uint32_t size() const { return static_cast<uint32_t>(std::abs(m_iend)) - m_ibegin; }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::descriptor
