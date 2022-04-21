#pragma once

#include "statefultask/AIStatefulTask.h"
#ifdef CWDEBUG
#include "debug/DebugSetName.h"
#endif

namespace vulkan {

class AsyncTask : public AIStatefulTask
{
 protected:
  using AIStatefulTask::AIStatefulTask;
  using direct_base_type = AIStatefulTask;

#ifdef CWDEBUG
  Ambifix debug_name_prefix(std::string prefix);
#endif
};

} // namespace vulkan
