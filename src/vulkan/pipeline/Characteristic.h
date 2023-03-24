#pragma once

#include "CharacteristicRange.h"

// Pipeline Characteristic base class for non-range characteristics.
//
// Usage:
//
//
// Note: if for some reason one needs a non-range Characteristic
// to be task anyway, then derive from task::CharacteristicRange
// directly. For example:
//
//   class MyPipelineCharacteristic : public vulkan::task::CharacteristicRange,
//       public vulkan::pipeline::AddVertexShader,      // Optional
//       public vulkan::pipeline::AddFragmentShader     // Optional
//                                                      // etc.
//   {
//    private:
//     ...
//
//    protected:
//     using direct_base_type = vulkan::task::CharacteristicRange;
//
//     // The different states of this task.
//     enum MyPipelineCharacteristic_state_type {
//       MyPipelineCharacteristic_initialize = direct_base_type::state_end
//     };
//
//     ~MyPipelineCharacteristic() override
//     {
//       DoutEntering(dc::vulkan, "MyPipelineCharacteristic::~MyPipelineCharacteristic() [" << this << "]");
//     }
//
//    public:
//     static constexpr state_type state_end = MyPipelineCharacteristic_initialize + 1;
//
//     MyPipelineCharacteristic(vulkan::task::SynchronousWindow const* owning_window, [...] COMMA_CWDEBUG_ONLY(bool debug)) :
//       vulkan::task::CharacteristicRange(owning_window COMMA_CWDEBUG_ONLY(0, 1, debug)), [...]
//     {
//       // We are not a range and therefore to not require the do_fill signal.
//       m_needs_signals = 0;
//       // We do not have vertex buffers.
//       m_use_vertex_buffers = false;  // Optional (example)
//     }
//
//    protected:
//     char const* state_str_impl(state_type run_state) const override
//     {
//       switch(run_state)
//       {
//         AI_CASE_RETURN(MyPipelineCharacteristic_initialize);
//       }
//       return direct_base_type::state_str_impl(run_state);
//     }
//
//     void initialize_impl() override
//     {
//       set_state(MyPipelineCharacteristic_initialize);
//     }
//
//     void multiplex_impl(state_type run_state) override
//     {
//       switch (run_state)
//       {
//         case MyPipelineCharacteristic_initialize:
//         {
//           Window const* window = static_cast<Window const*>(m_owning_window);
//
//           // Define the pipeline.
//           ...
//
//           run_state = CharacteristicRange_initialized;
//           break;
//         }
//       }
//       direct_base_type::multiplex_impl(run_state);
//     }
//
//    public:
//  #ifdef CWDEBUG
//     void print_on(std::ostream& os) const override;
//  #endif
//   };

namespace vulkan {

namespace task {
class PipelineFactory;
} // namespace task

namespace pipeline {

class FactoryHandle;

class CharacteristicBase : protected task::CharacteristicRange
{
 private:
  // Should be implemented by the user class derived from Characteristic.
  virtual void initialize() = 0;

  void initialize_impl() final
  {
    initialize();
    set_state(CharacteristicRange_initialized);
  }

 protected:
  CharacteristicBase(vulkan::task::SynchronousWindow const* owning_window COMMA_CWDEBUG_ONLY(bool debug)) :
      task::CharacteristicRange(owning_window COMMA_CWDEBUG_ONLY(0, 1, debug))
  {
    // We are not a range and therefore to not require the do_fill signal.
    m_needs_signals = 0;
  }
};

class Characteristic :
#ifdef __clang__
  // Bug work around. Without this clang <= 15 things that the destructor of the virtual base class is private too.
  protected
#else
  private
#endif
  CharacteristicBase
{
  friend task::PipelineFactory;
  friend FactoryHandle;

 protected:
  // These need to be accessible by the derived class (and are not related to CharacteristicRange being a task).
  using CharacteristicBase::m_owning_window;
  using CharacteristicBase::m_flat_create_info;
  using CharacteristicBase::add_uniform_buffer;
  using CharacteristicBase::add_combined_image_sampler;

 public:
  using CharacteristicBase::set_needs_signals;

 public:
  using CharacteristicBase::CharacteristicBase;

 protected:
  template<typename T>
  void add_to_flat_create_info(std::vector<T> const& list)
  {
    m_flat_create_info->add(&list, *this);
  }
};

} // namespace pipeline
} // namespace vulkan
