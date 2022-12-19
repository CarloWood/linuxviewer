#include "sys.h"
#include "CharacteristicRange.h"
#include "PipelineFactory.h"
#include "shader_builder/ShaderResourceDeclaration.h"
#include "descriptor/CombinedImageSamplerUpdater.h"

namespace vulkan::pipeline {

char const* CharacteristicRange::state_str_impl(state_type run_state) const
{
  switch (run_state)
  {
    AI_CASE_RETURN(CharacteristicRange_initialized);
    AI_CASE_RETURN(CharacteristicRange_continue_or_terminate);
    AI_CASE_RETURN(CharacteristicRange_filled);
    AI_CASE_RETURN(CharacteristicRange_preprocessed);
    AI_CASE_RETURN(CharacteristicRange_compiled);
  }
  AI_NEVER_REACHED
}

char const* CharacteristicRange::condition_str_impl(condition_type condition) const
{
  switch (condition)
  {
    AI_CASE_RETURN(do_fill);
    AI_CASE_RETURN(do_preprocess);
    AI_CASE_RETURN(do_compile);
    AI_CASE_RETURN(do_terminate);
  }
  return direct_base_type::condition_str_impl(condition);
}

void CharacteristicRange::multiplex_impl(state_type run_state)
{
#ifdef CWDEBUG
  if (!(run_state == m_expected_state || run_state == CharacteristicRange_continue_or_terminate))
  {
    DoutFatal(dc::core, "While running " << state_str_impl(run_state) << ": state " << state_str_impl(m_continue_state) <<
        " should have ended with 'run_state = " << state_str_impl(m_expected_state) << "', or it forgot to call"
        " set_continue_state() with the correct value.");
  }
#endif
  // Must rotate between CharacteristicRange_filled, CharacteristicRange_preprocessed and CharacteristicRange_compiled.
  // The current value of m_continue_state is the state that should have ended with 'run_state = m_expected_state;',
  // where m_expected_state must be replaced with the literal state of the value that it has.
  // Or m_continue_state forgot to call set_continue_state with the correct value.
  ASSERT(run_state == m_expected_state || run_state == CharacteristicRange_continue_or_terminate);
  switch (run_state)
  {
    case CharacteristicRange_initialized:
      m_owning_factory->characteristic_range_initialized();
      set_state(CharacteristicRange_continue_or_terminate);
      Debug(m_expected_state = CharacteristicRange_filled);
      wait(do_fill|do_terminate);             // Wait until we're ready to start the next fill index.
      break;
    case CharacteristicRange_filled:
      // This characteristic isn't derived from CharacteristicRange!?
      ASSERT((m_needs_signals & do_fill));
      m_owning_factory->characteristic_range_filled(m_characteristic_range_index);
      set_state(CharacteristicRange_continue_or_terminate);
      if ((m_needs_signals & do_preprocess))
      {
        Debug(m_expected_state = CharacteristicRange_preprocessed);
        wait(do_preprocess|do_terminate);       // Wait until we're ready to start preprocessing.
        break;
      }
      // If this Characteristic didn't need to do preprocessing, then why would it need compiling?!
      ASSERT(!(m_needs_signals & do_compile));
      Debug(m_expected_state = CharacteristicRange_filled);
      [[fallthrough]];
    case CharacteristicRange_continue_or_terminate:
      if (m_terminate)
      {
        finish();
        break;
      }
      set_state(m_continue_state);            // The continue state is the fill, preprocess or compile state of the derived class.
      break;
    case CharacteristicRange_preprocessed:
      // If this is a AddPushConstant then now we can copy AddPushConstant::m_sorted_push_constant_ranges to AddPushConstant::m_push_constant_ranges.
      copy_push_constant_ranges(m_owning_factory);
      // If this is a AddVertexShader then ...
      //   copy AddVertexShader::m_vertex_shader_input_sets to to AddVertexShader::m_vertex_input_binding_descriptions.
      copy_vertex_input_binding_descriptions();
      //   copy AddVertexShader::m_glsl_id_full_to_vertex_attribute to AddVertexShader::m_vertex_input_attribute_descriptions.
      copy_vertex_input_attribute_descriptions();
      // If this asserts then this characteristic isn't derived from AddShaderStage,
      // therefore we shouldn't have just "finished preprocessing".
      ASSERT((m_needs_signals & do_preprocess));
      m_owning_factory->characteristic_range_preprocessed();
      set_state(CharacteristicRange_continue_or_terminate);
      Debug(m_expected_state = CharacteristicRange_compiled);
      wait(do_compile|do_terminate);          // Wait until we're ready to start compiling.
      break;
    case CharacteristicRange_compiled:
      // If this asserts then this characteristic isn't derived from AddShaderStage,
      // therefore we shouldn't have just "finished compiling".
      ASSERT((m_needs_signals & do_preprocess));
      m_owning_factory->characteristic_range_compiled();
      if (!(m_needs_signals & do_fill))
      {
        // This must be a Characteristic (not a Range), so we're done.
        finish();
        break;
      }
      set_state(CharacteristicRange_continue_or_terminate);
      Debug(m_expected_state = CharacteristicRange_filled);
      wait(do_fill|do_terminate);             // Wait until we're ready for the next fill index state again.
      break;
  }
}

void CharacteristicRange::initialize_impl()
{
  // Start by default with the first state of the derived class.
  set_state(state_end);
}

char const* Characteristic::state_str_impl(state_type run_state) const
{
  switch(run_state)
  {
    AI_CASE_RETURN(Characteristic_initialized);
    AI_CASE_RETURN(Characteristic_preprocessed);
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
      // Characteristic is not supposed to use the do_fill signal?!
      ASSERT(!(m_needs_signals & do_fill));
      if ((m_needs_signals & do_preprocess))
      {
        set_state(CharacteristicRange_continue_or_terminate);
        wait(do_preprocess|do_terminate);
        return;
      }
      // If this Characteristic didn't need to do preprocessing, then why would it need compiling?!
      ASSERT(!(m_needs_signals & do_compile));
      finish();
      return;
  }
  direct_base_type::multiplex_impl(run_state);
}

void Characteristic::initialize_impl()
{
  // Start by default with the first state of the derived class.
  set_state(state_end);
}

} // namespace vulkan::pipeline
