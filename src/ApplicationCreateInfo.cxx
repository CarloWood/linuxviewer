#include "sys.h"

#ifdef CWDEBUG
#include "ApplicationCreateInfo.h"
#include <iostream>

void ApplicationCreateInfo::print_on(std::ostream& os) const
{
  os << '{';
  os << "max_duration:" << max_duration << ", ";
  os << "number_of_threads:" << number_of_threads << ", ";
  os << "max_number_of_threads:" << max_number_of_threads << ", ";
  os << "queue_capacity:" << queue_capacity << ", ";
  os << "reserved_threads:" << reserved_threads << ", ";
  os << "application_name:\"" << application_name << "\"";
  os << '}';
}
#endif
