#include "MemoryTypeBitsPrinter.h"
#include <vulkan/vulkan.hpp>
#include <iosfwd>

namespace vulkan {
class LogicalDevice;
} // namespace vulkan

namespace vk_utils {

struct MemoryRequirementsPrinter
{
  MemoryTypeBitsPrinter m_memory_type_bits_printer;
  uint32_t m_memory_heap_count;

  void operator()(std::ostream& os, vk::MemoryRequirements const& memory_requirements) const;
};

} // namespace vk_utils

