#include "sys.h"
#include "AsyncTask.h"

namespace vulkan {

#ifdef CWDEBUG
Ambifix AsyncTask::debug_name_prefix(std::string prefix)
{
  return { std::move(prefix), as_postfix(this) };
}
#endif

} // namespace vulkan
