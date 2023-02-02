#pragma once

#include "statefultask/AIStatefulTask.h"

namespace vulkan {

class Application;
class Ambifix;

namespace task {

// A task that runs asynchronously from the render loop - for example from the thread pool
// or in immediate mode.
class AsyncTask : public AIStatefulTask
{
 protected:
  using AIStatefulTask::AIStatefulTask;
  using direct_base_type = AIStatefulTask;

#ifdef CWDEBUG
  Ambifix debug_name_prefix(std::string prefix);
#endif
};

} // namespace task
} // namespace vulkan
