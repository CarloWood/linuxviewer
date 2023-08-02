#ifndef VULKAN_SEMAPHORE_WATCHER_H
#define VULKAN_SEMAPHORE_WATCHER_H

#include "AsyncTask.h"
#include "SynchronousTask.h"
#include "utils/Vector.h"
#include <type_traits>

namespace vulkan {
class TimelineSemaphore;

namespace task {
using ::task::TaskType;

struct SemaphoreWatcherWatchData
{
  TimelineSemaphore const* m_timeline_semaphore;        // The timeline semaphore to watch.
  uint64_t m_signal_value;                                      // The counter value that it must reach before we can call signal on a task.

  // Clang requires a constructor.
  SemaphoreWatcherWatchData(TimelineSemaphore const* timeline_semaphore, uint64_t signal_value) :
    m_timeline_semaphore(timeline_semaphore), m_signal_value(signal_value) { }
};

struct SemaphoreWatcherValueTaskConditionTriplet
{
  uint64_t m_signal_value;                                      // The counter value that we must reach before we can signal m_task.
  AIStatefulTask* m_task;                                       // The task to wake up when the associated timeline semaphore reaches m_signal_value.
  AIStatefulTask::condition_type m_condition;                   // The condition to use.

  SemaphoreWatcherValueTaskConditionTriplet(uint64_t signal_value, AIStatefulTask* task, AIStatefulTask::condition_type condition) :
    m_signal_value(signal_value), m_task(task), m_condition(condition) { }
};

struct SemaphoreWatcherNotifyData
{
  std::vector<SemaphoreWatcherValueTaskConditionTriplet> m_value_task_condition_triplets;                // Sorted list of value / task / condition triplets (increasing signal values).

  // Used for the first entry.
  SemaphoreWatcherNotifyData(uint64_t signal_value, AIStatefulTask* task, AIStatefulTask::condition_type condition)
  {
    m_value_task_condition_triplets.emplace_back(signal_value, task, condition);
  }
};

using SemaphoreWatcherIndex = utils::VectorIndex<SemaphoreWatcherWatchData>;

struct SemaphoreWatcherWatchSet
{
  utils::Vector<SemaphoreWatcherWatchData> m_watch_data;                                // List with semaphores and counter values that we are waiting for.
  utils::Vector<SemaphoreWatcherNotifyData, SemaphoreWatcherIndex> m_notify_data;       // List with associated notify data.
};

template<TaskType BASE>
class SemaphoreWatcher : public BASE
{
 public:
  static constexpr AIStatefulTask::condition_type have_semaphores = 1;

 private:
  using watch_set_type = threadsafe::Unlocked<SemaphoreWatcherWatchSet, threadsafe::policy::Primitive<std::mutex>>;
  watch_set_type m_watch_set;

  void remove(watch_set_type::wat const& watch_set_w, SemaphoreWatcherIndex wsi, SemaphoreWatcherIndex wsi_last)
  {
    Dout(dc::notice(BASE::mSMDebug), "SemaphoreWatcher::remove() removing timeline semaphore " << watch_set_w->m_watch_data[wsi].m_timeline_semaphore);
    if (wsi != wsi_last)
    {
      watch_set_w->m_watch_data[wsi] = watch_set_w->m_watch_data[wsi_last];
      watch_set_w->m_notify_data[wsi] = std::move(watch_set_w->m_notify_data[wsi_last]);
    }
    watch_set_w->m_watch_data.pop_back();
    watch_set_w->m_notify_data.pop_back();
  }

 protected:
  enum semaphore_watcher_state_type {
    SemaphoreWatcher_poll = BASE::direct_base_type::state_end,
    SemaphoreWatcher_done
  };

 public:
  static typename BASE::state_type constexpr state_end = SemaphoreWatcher_done + 1;

  using BASE::BASE;
  using state_type = typename BASE::state_type;
  using condition_type = typename BASE::condition_type;
  using BASE::set_state;
  using BASE::yield;
  using BASE::wait;
  using BASE::signal;

  void add(TimelineSemaphore const* timeline_semaphore, uint64_t signal_value, AIStatefulTask* task, AIStatefulTask::condition_type condition);
  void remove(TimelineSemaphore const* timeline_semaphore);
  bool poll();

 protected:
  ~SemaphoreWatcher() override = default;

  // Implementation of virtual functions of AIStatefulTask.
  char const* condition_str_impl(condition_type condition) const override;
  char const* state_str_impl(state_type run_state) const override;
  char const* task_name_impl() const override;
  void multiplex_impl(state_type run_state) override;
  void initialize_impl() override;
};

class AsyncSemaphoreWatcher : public SemaphoreWatcher<AsyncTask>
{
 public:
  static constexpr AIStatefulTask::condition_type poll_timer = 2;

 private:
  threadpool::Timer::Interval m_poll_rate_interval{threadpool::Interval<4, std::chrono::milliseconds>{}};      // The minimum time between two polls.
  threadpool::Timer m_poll_rate_limiter{[this](){ signal(poll_timer); }};

 public:
  using SemaphoreWatcher<AsyncTask>::SemaphoreWatcher;

 protected:
  char const* condition_str_impl(condition_type condition) const override;
  void multiplex_impl(state_type run_state) override;
};

} // namespace task
} // namespace vulkan

#endif // VULKAN_SEMAPHORE_WATCHER_H

#ifndef VULKAN_TIMELINE_SEMAPHORE_H
#include "TimelineSemaphore.h"
#endif

#ifndef VULKAN_SEMAPHORE_WATCHER_H_definitions
#define VULKAN_SEMAPHORE_WATCHER_H_definitions

namespace vulkan::task {

template<TaskType BASE>
void SemaphoreWatcher<BASE>::add(TimelineSemaphore const* timeline_semaphore, uint64_t signal_value, AIStatefulTask* task, AIStatefulTask::condition_type condition)
{
  DoutEntering(dc::notice(BASE::mSMDebug), "SemaphoreWatcher::add(" << timeline_semaphore << ", " << signal_value << ", " << task << ", " << task->print_conditions(condition) << ")");
  watch_set_type::wat watch_set_w(m_watch_set);
  SemaphoreWatcherIndex const wsi_end{watch_set_w->m_watch_data.size()};
  for (SemaphoreWatcherIndex wsi{0}; wsi != wsi_end; ++wsi)
    if (watch_set_w->m_watch_data[wsi].m_timeline_semaphore == timeline_semaphore)
    {
      // Each subsequent signal_value must be greater or equal the last m_signal_value.
      ASSERT(!watch_set_w->m_notify_data[wsi].m_value_task_condition_triplets.empty() &&
          signal_value >= watch_set_w->m_notify_data[wsi].m_value_task_condition_triplets.back().m_signal_value);
      // Append to SemaphoreWatcherWatchSet::m_notify_data[wsi].m_value_task_condition_triplets a SemaphoreWatcherValueTaskConditionTriplet,
      watch_set_w->m_notify_data[wsi].m_value_task_condition_triplets.emplace_back(signal_value, task, condition);
      return;
    }
  // New semaphore.
  watch_set_w->m_watch_data.emplace_back(timeline_semaphore, signal_value);
  watch_set_w->m_notify_data.emplace_back(signal_value, task, condition);
  signal(have_semaphores);
}

template<TaskType BASE>
void SemaphoreWatcher<BASE>::remove(TimelineSemaphore const* timeline_semaphore)
{
  DoutEntering(dc::notice(BASE::mSMDebug), "SemaphoreWatcher::remove(" << timeline_semaphore << ")");
  watch_set_type::wat watch_set_w(m_watch_set);
  SemaphoreWatcherIndex const wsi_end{watch_set_w->m_watch_data.size()};
  for (SemaphoreWatcherIndex wsi{0}; wsi != wsi_end; ++wsi)
    if (watch_set_w->m_watch_data[wsi].m_timeline_semaphore == timeline_semaphore)
    {
      remove(watch_set_w, wsi, wsi_end - 1);
      return;
    }
  // Could not find the semaphore; assume this means it was signaled, polled and removed before
  // we managed to get the lock, and do nothing.
}

template<TaskType BASE>
bool SemaphoreWatcher<BASE>::poll()
{
  DoutEntering(dc::notice(BASE::mSMDebug), "SemaphoreWatcher::poll()");
  watch_set_type::wat watch_set_w(m_watch_set);
  SemaphoreWatcherIndex wsi_end{watch_set_w->m_watch_data.size()};
  for (SemaphoreWatcherIndex wsi{0}; wsi != wsi_end;)
  {
    uint64_t const signal_value = watch_set_w->m_watch_data[wsi].m_signal_value;
    uint64_t const counter_value = watch_set_w->m_watch_data[wsi].m_timeline_semaphore->get_counter_value();
    Dout(dc::notice(BASE::mSMDebug), "Timeline semaphore " << watch_set_w->m_watch_data[wsi].m_timeline_semaphore << " has value " << counter_value << '.');
    if (counter_value >= signal_value)
    {
      auto value_task_condition_triplet = watch_set_w->m_notify_data[wsi].m_value_task_condition_triplets.begin();
      while (value_task_condition_triplet != watch_set_w->m_notify_data[wsi].m_value_task_condition_triplets.end())
      {
        if (counter_value < value_task_condition_triplet->m_signal_value)
          break;
        value_task_condition_triplet->m_task->signal(value_task_condition_triplet->m_condition);
        ++value_task_condition_triplet;
      }
      // It is not likely that we're waiting on multiple values of the same semaphore.
      if (AI_UNLIKELY(value_task_condition_triplet != watch_set_w->m_notify_data[wsi].m_value_task_condition_triplets.end()))
      {
        watch_set_w->m_notify_data[wsi].m_value_task_condition_triplets.erase(watch_set_w->m_notify_data[wsi].m_value_task_condition_triplets.begin(), value_task_condition_triplet);
        watch_set_w->m_watch_data[wsi].m_signal_value = value_task_condition_triplet->m_signal_value;
        ++wsi;
        continue;
      }
      remove(watch_set_w, wsi, --wsi_end);      // This removes a single element: that at index wsi.
      // Paranoia check.
      ASSERT(wsi_end == SemaphoreWatcherIndex{watch_set_w->m_watch_data.size()});
      continue;
    }
    ++wsi;
  }
  return !watch_set_w->m_watch_data.empty();
}

template<TaskType BASE>
char const* SemaphoreWatcher<BASE>::condition_str_impl(typename BASE::condition_type condition) const
{
  switch (condition)
  {
    AI_CASE_RETURN(have_semaphores);
  }
  return BASE::condition_str_impl(condition);
}

template<TaskType BASE>
char const* SemaphoreWatcher<BASE>::state_str_impl(typename BASE::state_type run_state) const
{
  switch (run_state)
  {
    AI_CASE_RETURN(SemaphoreWatcher_poll);
    AI_CASE_RETURN(SemaphoreWatcher_done);
  }
  AI_NEVER_REACHED;
}

template<TaskType BASE>
char const* SemaphoreWatcher<BASE>::task_name_impl() const
{
  if constexpr (std::is_same_v<BASE, SynchronousTask>)
    return "SemaphoreWatcher<SynchronousTask>";
  else if constexpr (std::is_same_v<BASE, AsyncTask>)
    return "SemaphoreWatcher<AsyncTask>";
  return "SemaphoreWatcher<>";
}

template<TaskType BASE>
void SemaphoreWatcher<BASE>::multiplex_impl(typename BASE::state_type run_state)
{
  switch (run_state)
  {
    case SemaphoreWatcher_poll:
      if (poll())
        yield();
      else
        wait(have_semaphores);
      break;
    case SemaphoreWatcher_done:
      BASE::finish();
      break;
  }
}

template<TaskType BASE>
void SemaphoreWatcher<BASE>::initialize_impl()
{
  BASE::set_state(SemaphoreWatcher_poll);
}

} // namespace vulkan::task

#endif // VULKAN_SEMAPHORE_WATCHER_H_definitions
