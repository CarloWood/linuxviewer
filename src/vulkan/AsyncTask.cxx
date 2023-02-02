#include "sys.h"
#include "AsyncTask.h"
#ifdef CWDEBUG
#include "debug/DebugSetName.h"
#endif

namespace vulkan::task {

#ifdef CWDEBUG
Ambifix AsyncTask::debug_name_prefix(std::string prefix)
{
  return { std::move(prefix), as_postfix(this) };
}
#endif

} // namespace vulkan::task
