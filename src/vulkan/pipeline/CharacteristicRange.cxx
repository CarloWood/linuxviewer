#include "sys.h"
#include "CharacteristicRange.h"
#include "PipelineFactory.h"

namespace vulkan::pipeline {

char const* CharacteristicRange::state_str_impl(state_type run_state) const
{
  switch (run_state)
  {
    // A complete listing of my_task_state_type.
    AI_CASE_RETURN(CharacteristicRange_initialized);
    AI_CASE_RETURN(CharacteristicRange_check_terminate);
    AI_CASE_RETURN(CharacteristicRange_filled);
  }
  AI_NEVER_REACHED
}

char const* CharacteristicRange::condition_str_impl(condition_type condition) const
{
  switch (condition)
  {
    AI_CASE_RETURN(do_fill);
    AI_CASE_RETURN(do_terminate);
  }
  return direct_base_type::condition_str_impl(condition);
}

void CharacteristicRange::multiplex_impl(state_type run_state)
{
  switch (run_state)
  {
    case CharacteristicRange_initialized:
      m_owning_factory->characteristic_range_initialized();
      set_state(CharacteristicRange_check_terminate);
      wait(do_fill|do_terminate);
      break;
    case CharacteristicRange_check_terminate:
      if (m_terminate)
      {
        finish();
        break;
      }
      set_state(m_continue_state);
      break;
    case CharacteristicRange_filled:
      m_owning_factory->characteristic_range_filled(m_characteristic_range_index);
      set_state(CharacteristicRange_check_terminate);
      wait(do_fill|do_terminate);
      break;
  }
}

ShaderInputData& CharacteristicRange::shader_input_data() { return m_owning_factory->shader_input_data({}); }
ShaderInputData const& CharacteristicRange::shader_input_data() const { return m_owning_factory->shader_input_data({}); }

} // namespace vulkan::pipeline
