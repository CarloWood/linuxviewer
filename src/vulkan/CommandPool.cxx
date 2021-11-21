#include "sys.h"
#ifdef CWDEBUG
#include "CommandPool.h"
#include <atomic>
#endif

#ifdef CWDEBUG
namespace vulkan {
namespace details {

UniquePoolID get_unique_pool_id()
{
  static std::atomic<UniquePoolID> pool_id_tip;

  return ++pool_id_tip;
}

} // namespace details
} // namespace vulkan
#endif
