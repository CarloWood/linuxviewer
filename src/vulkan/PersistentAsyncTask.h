#pragma once

#include "AsyncTask.h"
#include "utils/threading/Gate.h"

namespace vulkan::task {

// A task that runs asynchronously from the render loop - and is long lived; which
// doesn't normally finish. These tasks need to be killed at the end of the application.
class PersistentAsyncTask : public AsyncTask
{
 private:
  // To stop the main thread from exiting until all persistent tasks have finished.
  static utils::threading::Gate s_until_persistent_tasks_terminated;    // This Gate is opened when the last PersistentAsyncTask is destructed.
  static std::atomic<int> s_task_count;                                 // The number of running PersistentAsyncTask's.

 protected:
  using direct_base_type = AsyncTask;

  PersistentAsyncTask(CWDEBUG_ONLY(bool debug));
  ~PersistentAsyncTask();

 public:
  static void terminate_and_wait();
};

} // namespace vulkan::task
