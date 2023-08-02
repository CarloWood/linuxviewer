#pragma once

#include "QueueRequestKey.h"
#include "ImmediateSubmitQueue.h"
#include "utils/UltraHash.h"
#include "threadsafe/threadsafe.h"
#include "threadsafe/AIReadWriteSpinLock.h"
#include <boost/intrusive_ptr.hpp>
#include <vector>
#include "debug.h"

namespace vulkan {

class QueuePool;
class LogicalDevice;

// This is the type of the unlocked QueuePool::s_map that used, by the static functions of QueuePool, to map QueueRequestKey's to QueuePool's.
struct QueuePoolMap
{
  // Fast mapping.
  utils::UltraHash ultra_hash;
  std::vector<QueuePool*> lookup_table;

  // We keep a slow copy of the map too inorder to recreate ultra_hash whenever a new key is added.
  std::vector<uint64_t> keys;           // keys[i] must map to pools[i].
  std::vector<QueuePool*> pools;

  QueuePoolMap() : lookup_table(64) { }
};

// This is the type of the unlocked QueuePool::m_tasks.
struct QueuePoolTasks
{
  std::vector<boost::intrusive_ptr<task::ImmediateSubmitQueue>> tasks;
};

class QueuePool
{
  // The type of the global s_map that maps keys to instances.
  using map_type = threadsafe::Unlocked<QueuePoolMap, threadsafe::policy::ReadWrite<AIReadWriteSpinLock>>;
  // The type of m_tasks.
  using tasks_type = threadsafe::Unlocked<QueuePoolTasks, threadsafe::policy::ReadWrite<AIReadWriteSpinLock>>;

 private:
  // Object that maps QueueRequestKey to QueuePool instance.
  static map_type s_map;
                                                // There should be a one-on-one relationship between the value of this key and QueuePool objects.
#if CW_DEBUG
  static bool s_cleaned_up;
#endif

  LogicalDevice const* m_logical_device;        // The corresponding logical device.
  uint64_t const m_key_as_uint64;               // The key that uniquely identifies this pool.
  std::atomic<bool> m_no_more_queues;           // Set when all available queues have been acquired. Once set we'll return the least busy task from m_tasks.
  std::atomic<int> m_next_task;                 // Rotates over the existing tasks. FIXME: we should use the task with an empty queue first and/or the smallest queue.
  tasks_type m_tasks;                           // A list with running task::ImmediateSubmitQueue pointers.
  std::mutex m_acquiring_queue;                 // Used in get_immediate_submit_queue_task.

 private:
  // Add key by reinitializing the UltraHash and lookup_table.
  static QueuePool& insert_key(map_type::rat& map_r, ImmediateSubmitRequest const& submit_request);

  [[gnu::always_inline]] static QueuePool* instance(map_type::rat const& map_r, uint64_t key_as_uint64)
  {
    // Convert the key into a lookup table index and grab the QueuePool* from it.
    QueuePool* const pool = map_r->lookup_table[map_r->ultra_hash.index(key_as_uint64)];
    if (AI_LIKELY(pool) && AI_LIKELY(pool->m_key_as_uint64 == key_as_uint64))  // The fast path is where key was inserted before.
      return pool;

    // Unlikely.
    return nullptr;
  }

 public:
  QueuePool(LogicalDevice const* logical_device, uint64_t key_as_uint64) : m_logical_device(logical_device), m_key_as_uint64(key_as_uint64), m_no_more_queues(false), m_next_task(0)
  {
    DoutEntering(dc::vulkan, "QueuePool::QueuePool(" << logical_device << ", 0x" << std::hex << key_as_uint64 << ") [" << this << "]");
  }

  ~QueuePool();

  static QueuePool& instance(ImmediateSubmitRequest const& submit_request)
  {
    // We make use of the fact that this hash is reversible.
    // This should be 0 clock cycles as it just aliases key.
    uint64_t const key_as_uint64 = submit_request.queue_request_key().as_uint64();

    for (;;)
    {
      try
      {
        map_type::rat map_r(s_map);

        // Convert the key into a lookup table index and grab the QueuePool* from it.
        QueuePool* pool = instance(map_r, key_as_uint64);
        if (AI_LIKELY(pool))
          return *pool;

        return insert_key(map_r, submit_request);
      }
      catch(std::exception const&)
      {
        s_map.rd2wryield();
      }
    }

    AI_NEVER_REACHED
  }

  static void clean_up();

  // Returns a pointer to a running ImmediateSubmitQueue from this pool.
  task::ImmediateSubmitQueue* get_immediate_submit_queue_task(CWDEBUG_ONLY(bool debug));
};

} // namespace vulkan
