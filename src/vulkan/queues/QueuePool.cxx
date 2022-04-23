#include "sys.h"
#include "QueuePool.h"
#include "ImmediateSubmitQueue.h"
#include "Exceptions.h"
#include "Application.h"
#include <algorithm>

namespace vulkan {

//static
QueuePool::map_type QueuePool::s_map;

#if CW_DEBUG
//static
bool QueuePool::s_cleaned_up = false;
#endif

//static
QueuePool& QueuePool::insert_key(ImmediateSubmitRequest const& submit_request)
{
  // QueuePool::clean_up should only be called after QueuePool will no longer be used at all.
  ASSERT(!s_cleaned_up);

  // Treat the key as an uint64_t.
  uint64_t const key_as_uint64 = submit_request.queue_request_key().as_uint64();

  // Grab write lock on s_map.
  map_type::wat map_w(s_map);

  // Because we had to release the read lock, it is possible that some other
  // thread inserted this key in the meantime; so we have to check again if it exists.
  QueuePool* pool = instance(map_w, key_as_uint64);
  if (AI_UNLIKELY(pool))
    return *pool;

  // Doesn't exist. We need to create a new pool.
  QueuePool* new_pool = new QueuePool(submit_request.logical_device(), key_as_uint64);

  // Insert the new key into the slow map with a newly allocated QueuePool.
  map_w->keys.push_back(key_as_uint64);
  map_w->pools.push_back(new_pool);

  // Reinitialize ultra_hash.
  int lookup_table_size = map_w->ultra_hash.initialize(map_w->keys);
  if (lookup_table_size > map_w->lookup_table.size())
    map_w->lookup_table.resize(lookup_table_size);
  std::fill(map_w->lookup_table.begin(), map_w->lookup_table.end(), nullptr);
  for (int i = 0; i < map_w->keys.size(); ++i)
    map_w->lookup_table[map_w->ultra_hash.index(map_w->keys[i])] = map_w->pools[i];

  return *new_pool;
}

//static
void QueuePool::clean_up()
{
  map_type::wat map_w(s_map);

  // Just free all memory.
  map_w->lookup_table.clear();
  map_w->keys.clear();
  for (QueuePool* pool : map_w->pools)
    delete pool;
  map_w->pools.clear();

#if CW_DEBUG
  // This is done just so that IF QueuePool::instance is called again we'll call QueuePool::insert_key and assert there.
  map_w->ultra_hash.initialize({});
  map_w->lookup_table.resize(64);
  s_cleaned_up = true;
#endif
}

task::ImmediateSubmitQueue* QueuePool::get_immediate_submit_queue_task(CWDEBUG_ONLY(bool debug))
{
  // The fast-path assumes we already acquired all queues - of course.
  if (AI_LIKELY(m_no_more_queues.load(std::memory_order::relaxed)))
  {
    // Obtain read lock on m_tasks.
    tasks_type::rat tasks_r(m_tasks);
    // For now just rotate over all tasks.
    return tasks_r->tasks[++m_next_task % tasks_r->tasks.size()].get();
  }

  // For now just get a new queue, regardless of whether or not already running tasks are busy or not.

  // Get a pointer to the task::ImmediateSubmitQueue associated with the vulkan::QueueRequestKey that we have.
  vulkan::Queue queue;
  try
  {
    queue = m_logical_device->acquire_queue(m_key_as_uint64);
    Dout(dc::always, "Obtained queue: " << queue);
  }
  catch (vulkan::OutOfQueues_Exception const& error)
  {
    m_no_more_queues.store(true, std::memory_order::relaxed);
    return get_immediate_submit_queue_task(debug);
  }
  auto immediate_submit_queue_task = statefultask::create<task::ImmediateSubmitQueue>(m_logical_device, queue COMMA_CWDEBUG_ONLY(debug));
  immediate_submit_queue_task->run(vulkan::Application::instance().medium_priority_queue());

  // Keep a copy of the task pointer.
  task::ImmediateSubmitQueue* new_task = immediate_submit_queue_task.get();

  // Add the new task to the vector with tasks.
  {
    // Obtain write lock on m_tasks.
    tasks_type::wat tasks_w(m_tasks);
    // Append the new task.
    tasks_w->tasks.emplace_back(std::move(immediate_submit_queue_task));
  }

  return new_task;
}

} // namespace vulkan
