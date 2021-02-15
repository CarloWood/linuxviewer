#include "sys.h"
#include "LookAt.h"
#ifdef CWDEBUG
#include <iostream>
#endif

#ifdef CWDEBUG
void LookAt::print_on(std::ostream& os) const
{
  os << "{look_at:" << m_look_at << '}';
}
#endif
