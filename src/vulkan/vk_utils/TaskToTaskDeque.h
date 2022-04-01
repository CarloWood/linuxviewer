#pragma once

#include "Concepts.h"
#include "statefultask/DefaultMemoryPagePool.h"
#include "statefultask/AIStatefulTask.h"
#include "utils/DequeAllocator.h"
#include <deque>

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
// The produces task can add more `Datum` objects by passing rvalue-references to `have_new_datum`.
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
template<vulkan::ConceptStatefulTask BASE, typename DATUM>
class TaskToTaskDeque : public BASE
{
 protected:
  using direct_base_type = TaskToTaskDeque<BASE, DATUM>;

 public:
  static constexpr AIStatefulTask::condition_type need_action = 1;
  using Datum = DATUM;

 private:
  using deque_allocator_type = utils::DequeAllocator<DATUM>;
  using container_type = std::deque<DATUM, deque_allocator_type>;
  using new_data_type = aithreadsafe::Wrapper<container_type, aithreadsafe::policy::Primitive<std::mutex>>;

  utils::NodeMemoryResource m_nmr{AIMemoryPagePool::instance()};
  utils::DequeAllocator<DATUM> m_datum_allocator{m_nmr};
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

 public:
  // Called by producer.
  void have_new_datum(DATUM&& datum);
  void set_producer_finished();         // After a call to this function, no more calls to have_new_datum will follow.

}; // MoveNewPipelines

template<vulkan::ConceptStatefulTask BASE, typename DATUM>
void TaskToTaskDeque<BASE, DATUM>::have_new_datum(DATUM&& datum)
{
  typename new_data_type::wat(m_new_data)->push_back(std::move(datum));
  BASE::signal(need_action);
}

template<vulkan::ConceptStatefulTask BASE, typename DATUM>
void TaskToTaskDeque<BASE, DATUM>::set_producer_finished()
{
  m_producer_finished.store(true, std::memory_order::release);
  BASE::signal(need_action);
}

} // namespace vk_utils
