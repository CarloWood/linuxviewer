#include "sys.h"
#include "CommandPool.h"
#include <atomic>

namespace vulkan {
namespace details {

UniquePoolID get_unique_pool_id()
{
  static std::atomic<UniquePoolID> pool_id_tip;

  return ++pool_id_tip;
}

} // namespace details
} // namespace vulkan
