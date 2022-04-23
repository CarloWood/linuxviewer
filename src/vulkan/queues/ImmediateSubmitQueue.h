#pragma once

#include "ImmediateSubmitRequest.h"
#include "AsyncTask.h"
#include "vk_utils/TaskToTaskDeque.h"

namespace task {

class ImmediateSubmitQueue final : public vk_utils::TaskToTaskDeque<vulkan::AsyncTask, vulkan::ImmediateSubmitRequest>
{
 public:
  static constexpr auto pool_type = vulkan::ImmediateSubmitRequest::pool_type;

 private:
  using command_pool_type = vulkan::CommandPool<pool_type>;
  command_pool_type m_command_pool;
  vulkan::Queue m_queue;

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
    // Arguments for m_command_pool.
    vulkan::LogicalDevice const* logical_device,
    vulkan::Queue const& queue
    COMMA_CWDEBUG_ONLY(bool debug = false));
};

} // namespace task
