#include <iosfwd>
#include <cstdint>

namespace vk_utils {

struct MemoryTypeBitsPrinter
{
  uint32_t m_memory_type_count;

  void operator()(std::ostream& os, uint32_t memory_type_bits) const;
};

} // namespace vk_utils

