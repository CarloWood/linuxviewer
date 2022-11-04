#pragma once

#include "utils/has_print_on.h"
#include <cassert>
#include <iosfwd>
#include <vector>

namespace vulkan::pipeline::partitions {
using utils::has_print_on::operator<<;

enum Infinite
{
  negative_inf,
  normal,
  positive_inf
};

class Score
{
 private:
  int m_number_of_positive_inf;
  double m_value;
  int m_number_of_negative_inf;

 public:
  Score(double value = 0.0) : m_number_of_positive_inf(0), m_value(value), m_number_of_negative_inf(0) { }
  Score(Infinite inf) : m_number_of_positive_inf((inf == positive_inf) ? 1 : 0), m_value(0.0), m_number_of_negative_inf((inf == negative_inf) ? 1 : 0) { }

  bool is_zero() const
  {
    return m_number_of_positive_inf == 0 && m_number_of_negative_inf == 0 && m_value == 0.0;
  }

  bool is_infinite() const
  {
    return m_number_of_positive_inf != 0 || m_number_of_negative_inf != 0;
  }

  bool is_positive_inf() const
  {
    return m_number_of_positive_inf > 0 && m_number_of_negative_inf == 0;
  }

  double value() const
  {
    // Don't call this when is_infinite returns true.
    assert(!is_infinite());
    return m_value;
  }

  void clear()
  {
    m_number_of_positive_inf = 0;
    m_value = 0.0;
    m_number_of_negative_inf = 0;
  }

  Score& operator+=(Score const& score)
  {
    m_number_of_positive_inf += score.m_number_of_positive_inf;
    m_value += score.m_value;
    m_number_of_negative_inf += score.m_number_of_negative_inf;
    return *this;
  }

  friend Score operator-(Score const& lhs, Score const& rhs)
  {
    Score result = lhs;
    result.m_number_of_positive_inf -= rhs.m_number_of_positive_inf;
    result.m_value -= rhs.m_value;
    result.m_number_of_negative_inf -= rhs.m_number_of_negative_inf;
    return result;
  }

  Score& operator=(Score score)
  {
    clear();
    return this->operator+=(score);
  }

  friend bool operator<(Score const& lhs, Score const& rhs)
  {
    if (lhs.m_number_of_negative_inf != rhs.m_number_of_negative_inf)
      return lhs.m_number_of_negative_inf > rhs.m_number_of_negative_inf;

    if (lhs.m_number_of_positive_inf != rhs.m_number_of_positive_inf)
      return lhs.m_number_of_positive_inf < rhs.m_number_of_positive_inf;

    return lhs.m_value < rhs.m_value;
  }

  friend bool operator>=(Score const& lhs, Score const& rhs)
  {
    return !(lhs < rhs);
  }

  friend bool operator>(Score const& lhs, Score const& rhs)
  {
    return rhs < lhs;
  }

  bool unchanged(Score const& rhs)
  {
    return m_number_of_positive_inf == rhs.m_number_of_positive_inf && m_number_of_negative_inf == rhs.m_number_of_negative_inf &&
      m_value == rhs.m_value;   // Assuming rhs is a literal copy of lhs, this will just return true.
  }


#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::pipeline::partitions
