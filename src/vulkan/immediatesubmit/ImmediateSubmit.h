#pragma once

#include "ImmediateSubmitData.h"

namespace task {

class ImmediateSubmit final : public AIStatefulTask
{
 private:
   vulkan::ImmediateSubmitData m_data;

 protected:
  using direct_base_type = AIStatefulTask;

  // The different states of the task.
  enum ImmediateSubmit_state_type {
    ImmediateSubmit_start = direct_base_type::state_end,
    ImmediateSubmit_done
  };

 public:
  static state_type constexpr state_end = ImmediateSubmit_done + 1;

  void set_queue_reply_id_and_record_function(vulkan::ImmediateSubmitData data) { m_data = data; }

 protected:
  ~ImmediateSubmit() override;

  char const* state_str_impl(state_type run_state) const override;
  void multiplex_impl(state_type run_state) override;
};

} // namespace task
