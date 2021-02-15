#include "sys.h"
#include "UUID.h"
#ifdef CWDEBUG
#include <iostream>
#endif

#ifdef CWDEBUG
void UUID::print_on(std::ostream& os) const
{
  os << '{' << *static_cast<boost::uuids::uuid const*>(this) << '}';
}
#endif
