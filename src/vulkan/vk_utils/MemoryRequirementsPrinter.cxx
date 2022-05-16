#include "sys.h"
#include "MemoryRequirementsPrinter.h"
#include "utils/print_using.h"
#include <vulkan/vulkan.hpp>
#include "debug.h"

namespace vk_utils {

#ifdef CWDEBUG
void MemoryRequirementsPrinter::operator()(std::ostream& os, vk::MemoryRequirements const& memory_requirements) const
{
  os << '{' << "size:" << memory_requirements.size <<
     ", alignment:" << memory_requirements.alignment <<
     ", memoryTypeBits:" << utils::print_using(memory_requirements.memoryTypeBits, m_memory_type_bits_printer) <<
     ", m_memory_heap_count:" << m_memory_heap_count;
  os << '}';
}
#endif

} // namespace vk_utils
