#pragma once

#include "descriptor/SetLayout.h"
#include "descriptor/SetLayoutBinding.h"
#include "threadsafe/aithreadsafe.h"
#include <vulkan/vulkan.hpp>
#ifdef CWDEBUG
#include "debug/DebugSetName.h"
#endif
#include "debug.h"

namespace task {
class SynchronousWindow;
} // namespace task

namespace vulkan::shader_builder {
class ShaderResourceDeclaration;

namespace shader_resource {

// Base class for shader resources.
class Base
{
  using sorted_descriptor_set_layouts_container_t = std::vector<descriptor::SetLayout>;
  using set_layout_bindings_container_t = std::vector<descriptor::SetLayoutBinding>;

 private:
  // 'mutable' because shader resource instances added with add_* (add_texture, add_uniform_buffer, etc)
  // will only be created ONCE - synchronizing PipelineFactory by m_create_access_mutex, so that only a single
  // PipelineFactory will create the shader resources while all other PipelineFactory's will be waiting for
  // that to be finished. The thread-safe functions that use this mutex aren't really 'const', but they
  // are thread-safe ;).
  mutable AIStatefulTaskMutex m_create_access_mutex;
  std::atomic_bool m_created_and_updated_descriptor_set{false};
  descriptor::SetKey m_descriptor_set_key;
  using set_layout_bindings_t = aithreadsafe::Wrapper<set_layout_bindings_container_t, aithreadsafe::policy::ReadWrite<AIReadWriteMutex>>;
  mutable set_layout_bindings_t m_set_layout_bindings;

#ifdef CWDEBUG
  Ambifix m_ambifix;

 protected:
  // Implementation of virtual function of Base.
  Ambifix const& ambifix() const
  {
    return m_ambifix;
  }

 public:
  void add_ambifix(Ambifix const& ambifix) { m_ambifix = ambifix(m_ambifix.object_name()); }
#endif

 public:
  Base(descriptor::SetKey descriptor_set_key COMMA_CWDEBUG_ONLY(char const* debug_name)) :
    m_descriptor_set_key(descriptor_set_key) COMMA_CWDEBUG_ONLY(m_ambifix(debug_name)) { }
  Base() = default;     // Constructs a non-sensical m_descriptor_set_key that must be ignored when moving the object (in place).

  // Ignore m_descriptor_set_key when move- assigning and/or copying; the target should already have the right key.
  Base(Base&& rhs)
  {
  }
  Base& operator=(Base&& rhs)
  {
    return *this;
  }

  // Disallow copy and move constructing.
  Base(Base const& rhs) = delete;

  //---------------------------------------------------------------------------
  // The call to create and update_descriptor_set must happen after acquire_lock returned true,
  // followed by a call to set_created and before the accompanied call to release_lock.

  bool acquire_lock(AIStatefulTask* task, AIStatefulTask::condition_type condition) /*thread-safe*/ const
  {
    return m_create_access_mutex.lock(task, condition);
  }

  void add_set_layout_binding(descriptor::SetLayoutBinding set_layout_binding) /*thread-safe*/ const
  {
    set_layout_bindings_t::wat set_layout_bindings_w(m_set_layout_bindings);
    set_layout_bindings_w->push_back(set_layout_binding);
  }

  void set_created_and_updated_descriptor_set()
  {
    m_created_and_updated_descriptor_set.store(true, std::memory_order::release);
  }

  void release_lock() /*thread-safe*/ const
  {
    m_create_access_mutex.unlock();
  }

  bool is_created_and_updated_descriptor_set() const
  {
    return m_created_and_updated_descriptor_set.load(std::memory_order::acquire);
  }

  virtual void create(task::SynchronousWindow const* owning_window) = 0;
  virtual void update_descriptor_set(task::SynchronousWindow const* owning_window, vk::DescriptorSet vh_descriptor_set, uint32_t binding) const = 0;
  virtual void ready() = 0;
  //---------------------------------------------------------------------------

  // Accessors.
  descriptor::SetKey descriptor_set_key() const { return m_descriptor_set_key; }
  set_layout_bindings_t::crat set_layout_bindings() const { return m_set_layout_bindings; }

#ifdef CWDEBUG
 public:
  virtual void print_on(std::ostream& os) const = 0;
#endif
};

} // namespace shader_resource
} // namespace vulkan::shader_builder
