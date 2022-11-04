#include "sys.h"
#include "Score.h"
#include "Element.h"
#include "ElementPair.h"
#include <iostream>

namespace vulkan::pipeline::partitions {

void Score::print_on(std::ostream& os) const
{
  if (m_number_of_negative_inf == 1 && m_number_of_positive_inf == 0 && m_value == 0.0)
    os << "-inf";
  else if (m_number_of_negative_inf == 0 && m_number_of_positive_inf == 1 && m_value == 0.0)
    os << "+inf";
  else
  {
    os << m_value;
    if (m_number_of_positive_inf)
      os << " + inf * " << m_number_of_positive_inf;
    if (m_number_of_negative_inf)
      os << " - inf * " << m_number_of_negative_inf;
  }
}

} // namespace vulkan::pipeline::partitions
