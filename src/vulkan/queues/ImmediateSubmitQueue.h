#pragma once

#include "Application.h"
#include "CommandBufferFactory.h"
#include "ImmediateSubmitRequest.h"
#include "PersistentAsyncTask.h"
#include "TimelineSemaphore.h"
#include "vk_utils/TaskToTaskDeque.h"
#include "statefultask/DefaultMemoryPagePool.h"

namespace task {

class ImmediateSubmitQueue final : public vk_utils::TaskToTaskDeque<vulkan::PersistentAsyncTask, vulkan::ImmediateSubmitRequest>
{
 private:
  using CommandBuffer = vulkan::CommandBufferFactory::resource_type;   // vulkan::handle::CommandBuffer 
  utils::DequeAllocator<CommandBuffer> m_deque_allocator{vulkan::Application::instance().deque512_nmr()};
  statefultask::ResourcePool<vulkan::CommandBufferFactory> m_command_buffer_pool;
  vulkan::Queue m_queue;                                // Queue that is owned by this task.
  vulkan::TimelineSemaphore m_semaphore;                // Timeline semaphore used for submitting to m_queue.

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
  void multiplex_impl(state_type run_state) override;

 public:
  ImmediateSubmitQueue(
    // Arguments for m_command_buffer_pool.
    vulkan::LogicalDevice const* logical_device,
    vulkan::Queue const& queue
    COMMA_CWDEBUG_ONLY(bool debug = false));

  void wait_for(uint64_t signal_value) { m_semaphore.wait_for(signal_value); }

  void terminate();
};

} // namespace task
