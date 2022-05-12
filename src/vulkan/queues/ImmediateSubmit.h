#pragma once

#include "ImmediateSubmitRequest.h"
#include "AsyncTask.h"
#include "debug.h"

namespace task {

class ImmediateSubmitQueue;

class ImmediateSubmit : public vulkan::AsyncTask
{
 public:
  static constexpr condition_type submit_finished = 1;

 protected:
  // Constructor, set_queue_request_key, set_record_function.
  vulkan::ImmediateSubmitRequest m_submit_request;
  // Constructor.
  state_type m_continue_state{ImmediateSubmit_done};
  // ImmediateSubmit_start.
  ImmediateSubmitQueue* m_immediate_submit_queue_task{};

  // The different states of the task.
  enum ImmediateSubmit_state_type {
    ImmediateSubmit_start = direct_base_type::state_end,
    ImmediateSubmit_done
  };

 public:
  static state_type constexpr state_end = ImmediateSubmit_done + 1;

  ImmediateSubmit(CWDEBUG_ONLY(bool debug));
  ImmediateSubmit(vulkan::ImmediateSubmitRequest&& submit_request, state_type continue_state COMMA_CWDEBUG_ONLY(bool debug));

  void set_queue_request_key(vulkan::QueueRequestKey queue_request_key) { m_submit_request.set_queue_request_key(queue_request_key); }
  void set_record_function(vulkan::ImmediateSubmitRequest::record_function_type&& record_function) { m_submit_request.set_record_function(std::move(record_function)); }

 protected:
  ~ImmediateSubmit() override;

  char const* state_str_impl(state_type run_state) const override;
  void multiplex_impl(state_type run_state) override;
};

} // namespace task
