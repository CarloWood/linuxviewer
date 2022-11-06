#include "sys.h"
#include "CharacteristicRange.h"
#include "PipelineFactory.h"

namespace vulkan::pipeline {

char const* CharacteristicRange::state_str_impl(state_type run_state) const
{
  switch (run_state)
  {
    // A complete listing of my_task_state_type.
    AI_CASE_RETURN(CharacteristicRange_initialize);
    AI_CASE_RETURN(CharacteristicRange_fill);
  }
  AI_NEVER_REACHED
}

void CharacteristicRange::multiplex_impl(state_type run_state)
{
  switch (run_state)
  {
    case CharacteristicRange_initialize:
      initialize();
      m_owning_factory->characteristic_range_initialized();
      set_state(CharacteristicRange_fill);
      wait(do_fill);
      break;
    case CharacteristicRange_fill:
      fill();
      m_owning_factory->characteristic_range_filled(m_characteristic_range_index);
      wait(do_fill);
      break;
  }
}

ShaderInputData& CharacteristicRange::shader_input_data() { return m_owning_factory->shader_input_data({}); }
ShaderInputData const& CharacteristicRange::shader_input_data() const { return m_owning_factory->shader_input_data({}); }

} // namespace vulkan::pipeline
