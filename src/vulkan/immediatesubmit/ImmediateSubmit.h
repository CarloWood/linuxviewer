#pragma once

#include "ImmediateSubmitData.h"
#include "AsyncTask.h"
#include "debug.h"

namespace task {

class ImmediateSubmit final : public vulkan::AsyncTask
{
 private:
  vulkan::ImmediateSubmitData m_submit_data;

 protected:
  // The different states of the task.
  enum ImmediateSubmit_state_type {
    ImmediateSubmit_start = direct_base_type::state_end,
    ImmediateSubmit_done
  };

 public:
  static state_type constexpr state_end = ImmediateSubmit_done + 1;

  ImmediateSubmit(CWDEBUG_ONLY(bool debug));
  ImmediateSubmit(vulkan::ImmediateSubmitData&& submit_data COMMA_CWDEBUG_ONLY(bool debug));

  void set_queue_request_key(vulkan::QueueRequestKey queue_request_key) { m_submit_data.set_queue_request_key(queue_request_key); }
  void set_record_function(vulkan::ImmediateSubmitData::record_function_type&& record_function) { m_submit_data.set_record_function(std::move(record_function)); }

 protected:
  ~ImmediateSubmit() override;

  char const* state_str_impl(state_type run_state) const override;
  void multiplex_impl(state_type run_state) override;
};

} // namespace task
