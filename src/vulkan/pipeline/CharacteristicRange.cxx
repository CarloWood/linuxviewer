#include "sys.h"
#include "CharacteristicRange.h"
#include "PipelineFactory.h"
#include "shader_builder/ShaderResourceDeclaration.h"
#include "descriptor/CombinedImageSamplerUpdater.h"

namespace vulkan::task {

char const* CharacteristicRange::state_str_impl(state_type run_state) const
{
  switch (run_state)
  {
    AI_CASE_RETURN(CharacteristicRange_initialized);
    AI_CASE_RETURN(CharacteristicRange_fill_or_terminate);
    AI_CASE_RETURN(CharacteristicRange_filled);
    AI_CASE_RETURN(CharacteristicRange_preprocess);
    AI_CASE_RETURN(CharacteristicRange_compile);
    AI_CASE_RETURN(CharacteristicRange_build_shaders);
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
    AI_CASE_RETURN(do_build_shaders);
    AI_CASE_RETURN(do_terminate);
  }
  return direct_base_type::condition_str_impl(condition);
}

void CharacteristicRange::multiplex_impl(state_type run_state)
{
  // You're not handling the state run_state in the derived class?
  ASSERT(run_state < state_end);
  switch (run_state)
  {
    case CharacteristicRange_initialized:
      copy_shader_variables();

      m_owning_factory->characteristic_range_initialized();

      // If this is not a range (m_needs_signals was set to 0) then
      // either continue with preprocessing if needed or finish.
      if (!(m_needs_signals & do_fill))
      {
        if ((m_needs_signals & do_preprocess))
        {
          set_state(CharacteristicRange_preprocess);
          wait(do_preprocess|do_terminate);
          return;
        }
        // If this Characteristic didn't need to do preprocessing, then why would it need compiling?!
        ASSERT(!(m_needs_signals & do_compile));
        finish();
        return;
      }

      set_state(CharacteristicRange_fill_or_terminate);
      wait(do_fill|do_terminate);             // Wait until we're ready to start the next fill index.
      break;
    case CharacteristicRange_filled:
      // This characteristic isn't derived from CharacteristicRange!?
      ASSERT((m_needs_signals & do_fill));
      m_owning_factory->characteristic_range_filled(m_characteristic_range_index);
      if ((m_needs_signals & do_preprocess))
      {
        set_state(CharacteristicRange_preprocess);
        wait(do_preprocess|do_terminate);       // Wait until we're ready to start preprocessing.
        break;
      }
      // If this Characteristic didn't need to do preprocessing, then why would it need compiling?!
      ASSERT(!(m_needs_signals & do_compile));
      set_state(CharacteristicRange_fill_or_terminate);
      wait(do_fill|do_terminate);               // Wait until we're ready for the next fill index state again.
      break;
    case CharacteristicRange_fill_or_terminate:
      if (m_terminate)
      {
        finish();
        break;
      }
      pre_fill_state();
      set_state(m_fill_state);                  // The fill state of the derived class.
      break;
    case CharacteristicRange_preprocess:
      if (m_terminate)
      {
        finish();
        break;
      }
      {
        // This should be an AddShaderStage. Call that to preprocess shader(s).
        vk::ShaderStageFlags preprocessed_stages = preprocess_shaders_and_realize_descriptor_set_layouts(m_owning_factory);
        // If this is a AddPushConstant then now we can copy AddPushConstant::m_sorted_push_constant_ranges
        // to AddPushConstant::m_push_constant_ranges.
        copy_push_constant_ranges(m_owning_factory);
        if ((preprocessed_stages & vk::ShaderStageFlagBits::eVertex))
        {
          // If this is a AddVertexShader then ...
          //   copy AddVertexShader::m_vertex_shader_input_sets to AddVertexShader::m_vertex_input_binding_descriptions.
          //   copy AddVertexShader::m_glsl_id_full_to_vertex_attribute to AddVertexShader::m_vertex_input_attribute_descriptions.
          update_vertex_input_descriptions();
        }
      }
      // If this asserts then this characteristic isn't derived from AddShaderStage,
      // therefore we shouldn't have just "finished preprocessing".
      ASSERT((m_needs_signals & do_preprocess));
      m_owning_factory->characteristic_range_preprocessed();
      set_state(CharacteristicRange_compile);
      wait(do_compile|do_terminate);          // Wait until we're ready to start compiling.
      break;
    case CharacteristicRange_compile:
      if (m_terminate)
      {
        finish();
        break;
      }
      start_build_shaders();
      set_state(CharacteristicRange_build_shaders);
      Dout(dc::statefultask(mSMDebug), "Falling through to state CharacteristicRange_build_shaders.");
      [[fallthrough]];
    case CharacteristicRange_build_shaders:
      // If build_shaders returns false we must reenter it when signal(do_build_shaders) is received.
      // Note that it keeps returning false until all shaders are compiled.
      if (!build_shaders(this, m_owning_factory, do_build_shaders))
      {
        wait(do_build_shaders);
        break;
      }
      set_state(CharacteristicRange_compiled);
      Dout(dc::statefultask(mSMDebug), "Falling through to state CharacteristicRange_compiled.");
      [[fallthrough]];
    case CharacteristicRange_compiled:
      // If this asserts then this characteristic isn't derived from AddShaderStage,
      // therefore we shouldn't have just "finished compiling".
      ASSERT((m_needs_signals & do_compile));
      m_owning_factory->characteristic_range_compiled();
      if (!(m_needs_signals & do_fill))
      {
        // This must be a Characteristic (not a Range), so we're done.
        finish();
        break;
      }
      set_state(CharacteristicRange_fill_or_terminate);
      wait(do_fill|do_terminate);             // Wait until we're ready for the next fill index state again.
      break;
  }
}

void CharacteristicRange::initialize_impl()
{
  // Start by default with the first state of the derived class.
  set_state(state_end);
}

} // namespace vulkan::task
