#pragma once

#include "Application.h"
#include "CommandBufferFactory.h"
#include "ImmediateSubmitRequest.h"
#include "PersistentAsyncTask.h"
#include "TimelineSemaphore.h"
#include "vk_utils/TaskToTaskDeque.h"
#include "statefultask/DefaultMemoryPagePool.h"

namespace vulkan::task {

class ImmediateSubmitQueue final : public vk_utils::TaskToTaskDeque<vulkan::PersistentAsyncTask, vulkan::ImmediateSubmitRequest>
{
 private:
  using CommandBufferHandle = vulkan::CommandBufferFactory::resource_type;    // vulkan::handle::CommandBuffer
  utils::DequeAllocator<CommandBufferHandle> m_deque_allocator{vulkan::Application::instance().deque512_nmr()};
  statefultask::ResourcePool<CommandBufferFactory> m_command_buffer_pool;
  Queue m_queue;                                                // Queue that is owned by this task.
  TimelineSemaphore m_semaphore;                                // Timeline semaphore used for submitting to m_queue.
  int m_pending_requests{};                                             // The number of command buffers that were submitted but were not signaled yet.
  container_type::const_iterator m_last_submitted;                      // Pointer to the last ImmediateSubmitRequest associated with the pending requests.
                                                                        // Only valid if m_pending_requests > 0.

  // The different states of the task.
  enum ImmediateSubmitQueue_state_type {
    ImmediateSubmitQueue_need_action = direct_base_type::state_end,
    ImmediateSubmitQueue_done
  };

 public:
  static state_type constexpr state_end = ImmediateSubmitQueue_done + 1;

 protected:
  ~ImmediateSubmitQueue() override;

  char const* state_str_impl(state_type run_state) const override;
  char const* task_name_impl() const override;
  void multiplex_impl(state_type run_state) override;
  void abort_impl() override;

 public:
  ImmediateSubmitQueue(
    // Arguments for m_command_buffer_pool.
    vulkan::LogicalDevice const* logical_device,
    Queue const& queue
    COMMA_CWDEBUG_ONLY(bool debug = false));

  void wait_for(uint64_t signal_value) { m_semaphore.wait_for(signal_value); }

  void terminate();
};

} // namespace vulkan::task
