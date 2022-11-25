#pragma once

#include "debug.h"

namespace vulkan::pipeline {

class ConsecutiveRange
{
 private:
  int m_begin;
  int m_end;

 public:
  // Create an uninitialized ConsecutiveRange.
  ConsecutiveRange() = default;

  ConsecutiveRange(int begin, int end) : m_begin(begin), m_end(end)
  {
    // Empty ranges are not allowed.
    ASSERT(begin < end);
  }

  // Accessors.
  int begin() const { return m_begin; }
  int end() const { return m_end; }

  void extend_subrange(int fill_index)
  {
    m_begin = std::min(m_begin, fill_index);
    m_end = std::max(m_end, fill_index + 1);
  }

  void extend_subrange(ConsecutiveRange subrange)
  {
    m_begin = std::min(m_begin, subrange.m_begin);
    m_end = std::max(m_end, subrange.m_end);
  }

  friend bool operator<(ConsecutiveRange const& lhs, ConsecutiveRange const& rhs)
  {
    return lhs.m_end <= rhs.m_begin;
  }

  bool overlaps(ConsecutiveRange const& subrange) const
  {
    return m_end > subrange.m_begin && subrange.m_end > m_begin;
  }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::pipeline
