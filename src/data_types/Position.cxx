#include "sys.h"
#include "Position.h"
#ifdef CWDEBUG
#include <iostream>
#endif

#ifdef CWDEBUG
void Position::print_on(std::ostream& os) const
{
  os << "{position:" << m_position << '}';
}
#endif
