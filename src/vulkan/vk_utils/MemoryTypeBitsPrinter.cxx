#include "sys.h"
#include "MemoryTypeBitsPrinter.h"
#include <iostream>
#include "debug.h"

namespace vk_utils {

#ifdef CWDEBUG
void MemoryTypeBitsPrinter::operator()(std::ostream& os, uint32_t memory_type_bits) const
{
  os << '{';
  char const* prefix = "";
  for (int memory_type_index = 0; memory_type_index < m_memory_type_count; ++memory_type_index)
  {
    if ((memory_type_bits & (1 << memory_type_index)) != 0)
    {
      os << prefix << memory_type_index;
      prefix = ", ";
    }
  }
  os << '}';
}
#endif

} // namespace vk_utils
