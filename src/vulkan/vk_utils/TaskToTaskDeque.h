#pragma once

#include "../Concepts.h"
#include "../Application.h"
#include "statefultask/DefaultMemoryPagePool.h"
#include "statefultask/AIStatefulTask.h"
#include "utils/DequeAllocator.h"
#include <deque>
#include <algorithm>

namespace vk_utils {

// If a producer task wants to pass objects of type Datum to a consumer task,
// then the consumer can use this base class.
//
// Usage:
//
// class MyConsumerTask final : public vk_utils::TaskToTaskDeque<AIStatefulTask /* or SynchronousTask */, Datum>
// {
//  ...
//
// See MoveNewPipelines for an example implementation.
//
// This class uses AIMemoryPagePool and therefore uses Application::m_mpp.
// It also uses utils::DequeAllocator which relies on Application::m_dmri.
//
// The producer task can add more `Datum` objects by passing rvalue-references to `have_new_datum`.
// After the last call to `have_new_datum` it should call `set_producer_finished` (this can be
// done immediately after that last call).
//
// Each time one of these functions is called, the signal `need_action`
// is emitted. The consumer task therefore must already have been running,
// or not wait for that signal before entering the `*_need_action` state
// which typically will look like this:
//
//  case MoveNewPipelines_need_action:
//    flush_new_data([this](Datum&& datum){ /* do something with datum */ });
//    if (producer_not_finished())
//      break;
//    set_state(MoveNewPipelines_done);
//    [[fallthrough]];
//  case MoveNewPipelines_done:
//
template<task::TaskType BASE, typename DATUM>
class TaskToTaskDeque : public BASE
{
 public:
  static constexpr AIStatefulTask::condition_type need_action = 1;
  using Datum = DATUM;

 protected:
  using direct_base_type = TaskToTaskDeque<BASE, DATUM>;                // Not ours, but that of the derived class.
  using deque_allocator_type = utils::DequeAllocator<DATUM>;
  using container_type = std::deque<DATUM, deque_allocator_type>;
  using new_data_type = aithreadsafe::Wrapper<container_type, aithreadsafe::policy::Primitive<std::mutex>>;

 private:
  utils::DequeAllocator<DATUM> m_datum_allocator{vulkan::Application::instance().deque512_nmr()};
  new_data_type m_new_data{m_datum_allocator};
  std::atomic_bool m_producer_finished = false;

 protected:
  using BASE::BASE;

  // Called by consumer (derived task).
  void flush_new_data(std::function<void(Datum&&)> lambda)
  {
    for (;;)
    {
      Datum datum;
      {
        typename new_data_type::wat new_data_w(m_new_data);
        if (new_data_w->empty())
          break;
        datum = std::move(new_data_w->front());
        new_data_w->pop_front();
      }
      lambda(std::move(datum));
    }
  }

  // Called by consumer (derived task).
  bool producer_not_finished()
  {
    bool producer_finished = m_producer_finished.load(std::memory_order::acquire);
    if (!producer_finished)
      BASE::wait(need_action);
    return !producer_finished;
  }

  // Return begin() and decrease n to the number of elements that are in the deque,
  // if that is less than the requested n.
  typename container_type::const_iterator front_n(int& n) const
  {
    typename new_data_type::crat new_data_r(m_new_data);
    n = std::min((size_t)n, new_data_r->size());
    return new_data_r->begin();
  }

  // Erase all elements up till and including last, which must be the iterator
  // returned by front_n incremented n - 1 times.
  //
  // No other threads may have called pop_front_n in the meantime; in other
  // words the pair front_n/pop_front_n is single-threaded.
  //
  // Note that it is not possible to have the caller pass last + 1, because
  // the deque is not locked at the moment of the call and other threads
  // are allowed to call have_new_datum that will call push_back which
  // invalidates the end iterator; and last + 1 CAN be one past the end.
  void pop_front_n(typename container_type::const_iterator last)
  {
    typename new_data_type::wat new_data_w(m_new_data);
    new_data_w->erase(new_data_w->begin(), ++last);
  }

  // Erase the first element.
  void pop_front()
  {
    typename new_data_type::wat new_data_w(m_new_data);
    new_data_w->pop_front();
  }

 public:
  // Called by producer.
  void have_new_datum(DATUM&& datum);
  void set_producer_finished();         // After a call to this function, no more calls to have_new_datum will follow.

 protected:
  char const* condition_str_impl(AIStatefulTask::condition_type condition) const override
  {
    switch (condition)
    {
      AI_CASE_RETURN(need_action);
    }
    return BASE::condition_str_impl(condition);
  }
};

template<task::TaskType BASE, typename DATUM>
void TaskToTaskDeque<BASE, DATUM>::have_new_datum(DATUM&& datum)
{
  typename new_data_type::wat(m_new_data)->push_back(std::move(datum));
  BASE::signal(need_action);
}

template<task::TaskType BASE, typename DATUM>
void TaskToTaskDeque<BASE, DATUM>::set_producer_finished()
{
  m_producer_finished.store(true, std::memory_order::release);
  BASE::signal(need_action);
}

} // namespace vk_utils
