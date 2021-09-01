#include "sys.h"

#ifdef CWDEBUG
#include "ApplicationCreateInfo.h"
#include "vulkan.old//debug_ostream_operators.h"
#include <iostream>

void ApplicationCreateInfo::print_on(std::ostream& os) const
{
  os << *static_cast<vulkan::ApplicationInfo const*>(this);
  os << '{';
  os << "block_size:" << block_size << ", ";
  os << "minimum_chunk_size:" << minimum_chunk_size << ", ";
  os << "maximum_chunk_size:" << maximum_chunk_size << ", ";
  os << "max_duration:" << max_duration << ", ";
  os << "number_of_threads:" << number_of_threads << ", ";
  os << "max_number_of_threads:" << max_number_of_threads << ", ";
  os << "queue_capacity:" << queue_capacity << ", ";
  os << "reserved_threads:" << reserved_threads;
  os << '}';
}
#endif
