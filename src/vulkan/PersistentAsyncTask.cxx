#include "sys.h"
#include "PersistentAsyncTask.h"
#include "queues/QueuePool.h"

namespace vulkan {

utils::threading::Gate PersistentAsyncTask::s_until_persistent_tasks_terminated;
std::atomic<int> PersistentAsyncTask::s_task_count;

PersistentAsyncTask::PersistentAsyncTask(CWDEBUG_ONLY(bool debug)) : AsyncTask(CWDEBUG_ONLY(debug))
{
  // Keep track of the number of running PersistentAsyncTask's.
  s_task_count.fetch_add(1, std::memory_order_relaxed);
}

PersistentAsyncTask::~PersistentAsyncTask()
{
  // Open the s_until_persistent_tasks_terminated gate as soon as the last PersistentAsyncTask is destroyed.
  if (s_task_count.fetch_sub(1, std::memory_order_release) == 1)
  {
    std::atomic_thread_fence(std::memory_order_acquire);
    Dout(dc::notice, "Last PersistentAsyncTask was destructed. Terminating application.");
    s_until_persistent_tasks_terminated.open();
  }
}

//static
void PersistentAsyncTask::terminate_and_wait()
{
  DoutEntering(dc::vulkan, "PersistentAsyncTask::terminate_and_wait()");
  QueuePool::clean_up();
  // There might be no PersistentAsyncTask's at all. In that case we shouldn't wait for one to finish.
  if (s_task_count.load(std::memory_order_relaxed) > 0)
    s_until_persistent_tasks_terminated.wait();
}

} // namespace vulkan
