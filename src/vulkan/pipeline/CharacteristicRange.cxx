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
    AI_CASE_RETURN(CharacteristicRange_continue_or_terminate);
    AI_CASE_RETURN(CharacteristicRange_filled);
    AI_CASE_RETURN(CharacteristicRange_compiled);
  }
  AI_NEVER_REACHED
}

char const* CharacteristicRange::condition_str_impl(condition_type condition) const
{
  switch (condition)
  {
    AI_CASE_RETURN(do_fill);
    AI_CASE_RETURN(do_compile);
    AI_CASE_RETURN(do_terminate);
  }
  return direct_base_type::condition_str_impl(condition);
}

void CharacteristicRange::multiplex_impl(state_type run_state)
{
  // Must alternate between CharacteristicRange_filled and CharacteristicRange_compiled.
  // The current value of m_continue_state is the state that should have ended with 'run_state = m_expected_state;',
  // where m_expected_state must be replaced with the literal state of the value that it has.
  ASSERT(run_state == m_expected_state || run_state == CharacteristicRange_continue_or_terminate);
  switch (run_state)
  {
    case CharacteristicRange_initialized:
      m_owning_factory->characteristic_range_initialized();
      set_state(CharacteristicRange_continue_or_terminate);
      Debug(m_expected_state = CharacteristicRange_filled);
      wait(do_fill|do_terminate);
      break;
    case CharacteristicRange_continue_or_terminate:
      if (m_terminate)
      {
        finish();
        break;
      }
      set_state(m_continue_state);            // The continue state is the fill or compile state of the derived class.
      break;
    case CharacteristicRange_filled:
      m_owning_factory->characteristic_range_filled(m_characteristic_range_index);
      set_state(CharacteristicRange_continue_or_terminate);
      Debug(m_expected_state = CharacteristicRange_compiled);
      wait(do_compile|do_terminate);          // Wait until we're ready to start compiling.
      break;
    case CharacteristicRange_compiled:
      m_owning_factory->characteristic_range_compiled();
      set_state(CharacteristicRange_continue_or_terminate);
      Debug(m_expected_state = CharacteristicRange_filled);
      wait(do_fill|do_terminate);
      break;
  }
}

ShaderInputData& CharacteristicRange::shader_input_data() { return m_owning_factory->shader_input_data({}); }
ShaderInputData const& CharacteristicRange::shader_input_data() const { return m_owning_factory->shader_input_data({}); }

char const* Characteristic::state_str_impl(state_type run_state) const
{
  switch(run_state)
  {
    AI_CASE_RETURN(Characteristic_initialized);
    AI_CASE_RETURN(Characteristic_fill);
    AI_CASE_RETURN(Characteristic_compile_or_terminate);
    AI_CASE_RETURN(Characteristic_compiled);
  }
  return direct_base_type::state_str_impl(run_state);
}

void Characteristic::multiplex_impl(state_type run_state)
{
  switch (run_state)
  {
    case Characteristic_initialized:
      m_owning_factory->characteristic_range_initialized();
      set_state(Characteristic_fill);
      wait(do_fill|do_terminate);
      break;
    case Characteristic_fill:
      if (m_terminate)
      {
        finish();
        break;
      }
      // This not a range; nothing to fill (do that in initialize).
      m_owning_factory->characteristic_range_filled(m_characteristic_range_index);
      set_state(Characteristic_compile_or_terminate);
      wait(do_compile|do_terminate);          // Wait until we're ready to start compiling.
      break;
    case Characteristic_compile_or_terminate:
      if (m_terminate)
      {
        finish();
        break;
      }
      set_state(m_continue_state);            // The continue state is the compile state of the derived class.
      break;
    case Characteristic_compiled:
      m_owning_factory->characteristic_range_compiled();
      set_state(Characteristic_fill);
      wait(do_fill|do_terminate);
      break;
  }
}

} // namespace vulkan::pipeline
