#pragma once

#include "FrameResourceIndex.h"
#include "descriptor/FrameResourceCapableDescriptorSet.h"
#include "descriptor/SetLayout.h"
#include "descriptor/SetLayoutBinding.h"
#include "shader_builder/ShaderResourceIndex.h"
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
class Base;

// Helper class that stores the descriptor set allocation mutex and a vector with FrameResourceCapableDescriptorSet handles bound to
// the associated shader resource (m_debug_owner).
//
// Access to this class is protected by the read/write mutex on Base::m_set_layout_bindings_to_handles.
class SetMutexAndSetHandles
{
 private:
  AIStatefulTaskMutex m_mutex;                  // Mutex that is locked for the first shader resource of a given set a shader resources that we need a descriptor for.

  using descriptor_set_container_t = std::vector<descriptor::FrameResourceCapableDescriptorSet>;
  descriptor_set_container_t m_descriptor_sets; // Vector containing all descriptor sets that the shader resource owning this SetLayoutBinding (stored in shader_resource::Base::m_set_layout_bindings) is bound to.

#ifdef CWDEBUG
  AIStatefulTask const* m_debug_have_lock{};    // Pointer to the task that successfully locked m_mutex, or nullptr when unlocked.
  Base const* m_debug_owner;                    // The shader resource that has a map that contains this object.
#endif

 public:
#ifndef CWDEBUG
  // Construct a SetMutexAndSetHandles that contains no FrameResourceCapableDescriptorSet's.
  SetMutexAndSetHandles() = default;
  // Construct a SetMutexAndSetHandles that contains a single FrameResourceCapableDescriptorSet.
  SetMutexAndSetHandles(descriptor::FrameResourceCapableDescriptorSet&& descriptor_set) : m_descriptor_sets(std::move(descriptor_set)) { }
#else
  // Construct a SetMutexAndSetHandles that contains no FrameResourceCapableDescriptorSet's.
  SetMutexAndSetHandles(Base const* owner) : m_debug_owner(owner) { }
  // Construct a SetMutexAndSetHandles that contains a single FrameResourceCapableDescriptorSet.
  SetMutexAndSetHandles(descriptor::FrameResourceCapableDescriptorSet const& descriptor_set, Base const* owner) :
    m_descriptor_sets(1, descriptor_set), m_debug_owner(owner) { }
#endif

  void add_handle(descriptor::FrameResourceCapableDescriptorSet const& descriptor_set)
  {
    m_descriptor_sets.push_back(descriptor_set);
  }

#ifdef CWDEBUG
  bool lock_descriptor_sets(AIStatefulTask* task, AIStatefulTask::condition_type condition);
  void set_have_lock(AIStatefulTask* task);
#else
  bool lock_descriptor_sets(AIStatefulTask* task, AIStatefulTask::condition_type condition) { return m_mutex.lock(task, condition); }
#endif

  // Accessor.
  descriptor_set_container_t const& descriptor_sets(CWDEBUG_ONLY(AIStatefulTask const* task)) const
  {
    DoutEntering(dc::notice, "SetMutexAndSetHandles::descriptor_sets(" << task << ") [" << this << "]");
    return m_descriptor_sets;
  }

#ifdef CWDEBUG
  void unlock_descriptor_sets(AIStatefulTask const* task);
#else
  void unlock_descriptor_sets() { m_mutex.unlock(); }
#endif

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

// Base class for shader resources.
class Base
{
 public:
  using set_layout_bindings_to_handles_container_t = std::map<descriptor::SetLayoutBinding, SetMutexAndSetHandles>;

 private:
  // 'mutable' because shader resource instances added with add_* (add_texture, add_uniform_buffer, etc)
  // will only be created ONCE - synchronizing PipelineFactory by m_create_access_mutex, so that only a single
  // PipelineFactory will create the shader resources while all other PipelineFactory's will be waiting for
  // that to be finished. The thread-safe functions that use this mutex aren't really 'const', but they
  // are thread-safe ;).
  mutable AIStatefulTaskMutex m_create_access_mutex;
  std::atomic_bool m_created{false};
  descriptor::SetKey m_descriptor_set_key;
  using set_layout_bindings_to_handles_t = aithreadsafe::Wrapper<set_layout_bindings_to_handles_container_t, aithreadsafe::policy::ReadWrite<AIReadWriteMutex>>;
  mutable set_layout_bindings_to_handles_t m_set_layout_bindings_to_handles;

#ifdef CWDEBUG
 private:
  char const* m_debug_name;

 public:
  char const* debug_name() const { return m_debug_name; }
#endif

 public:
  Base(descriptor::SetKey descriptor_set_key COMMA_CWDEBUG_ONLY(char const* debug_name)) :
    m_descriptor_set_key(descriptor_set_key) COMMA_CWDEBUG_ONLY(m_debug_name(debug_name))
  {
    DoutEntering(dc::notice, "Base::Base(" << descriptor_set_key << ", " << debug::print_string(debug_name) << ") [" << this << "]");
  }
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

  bool acquire_create_lock(AIStatefulTask* task, AIStatefulTask::condition_type condition) /*thread-safe*/ const
  {
    DoutEntering(dc::notice, "Base::acquire_create_lock(" << task << ", " << task->print_conditions(condition) << ") [" << this << "]");
    return m_create_access_mutex.lock(task, condition);
  }

  bool lock_set_layout_binding(descriptor::SetLayoutBinding set_layout_binding, AIStatefulTask* task, AIStatefulTask::condition_type condition) /*thread-safe*/ const
  {
    DoutEntering(dc::notice, "lock_set_layout_binding(" << set_layout_binding << ", " << task << ", " << task->print_conditions(condition) << ")");
    set_layout_bindings_to_handles_t::wat set_layout_bindings_to_handles_w(m_set_layout_bindings_to_handles);
    auto pib = set_layout_bindings_to_handles_w->try_emplace(set_layout_binding COMMA_CWDEBUG_ONLY(this));  // Get existing or default construct SetMutexAndSetHandles.
    return pib.first->second.lock_descriptor_sets(task, condition);
  }

#ifdef CWDEBUG
  void set_have_lock(descriptor::SetLayoutBinding set_layout_binding, AIStatefulTask* task) /*thread-safe*/ const
  {
    DoutEntering(dc::notice, "set_have_lock(" << set_layout_binding << ", " << task << ")");
    set_layout_bindings_to_handles_t::wat set_layout_bindings_to_handles_w(m_set_layout_bindings_to_handles);
    auto iter = set_layout_bindings_to_handles_w->find(set_layout_binding);     // Get existing SetMutexAndSetHandles (see lock_set_layout_binding which created it when it didn't already exist then).
    ASSERT(iter != set_layout_bindings_to_handles_w->end());
    return iter->second.set_have_lock(task);
  }
#endif

  void add_set_layout_binding(descriptor::SetLayoutBinding set_layout_binding, descriptor::FrameResourceCapableDescriptorSet const& descriptor_set, AIStatefulTask* locked_by_task) /*thread-safe*/ const
  {
    DoutEntering(dc::notice, "add_set_layout_binding(" << set_layout_binding << ", " << descriptor_set << ", " << locked_by_task << ") for [" << this << "]");
    set_layout_bindings_to_handles_t::wat set_layout_bindings_to_handles_w(m_set_layout_bindings_to_handles);
    Dout(dc::notice, "m_set_layout_bindings_to_handles currently contains: " << *set_layout_bindings_to_handles_w);
    auto iter = set_layout_bindings_to_handles_w->find(set_layout_binding);
    if (iter == set_layout_bindings_to_handles_w->end())
    {
      Dout(dc::notice, "The key was not found, adding it...");
      CWDEBUG_ONLY(auto pib =) set_layout_bindings_to_handles_w->try_emplace(set_layout_binding, descriptor_set COMMA_CWDEBUG_ONLY(this));
      // We just couldn't find this key?!
      ASSERT(pib.second);
    }
    else
    {
      Dout(dc::notice, "This key was found, adding handle...");
      iter->second.add_handle(descriptor_set);
      if (locked_by_task)
      {
        iter->second.unlock_descriptor_sets(CWDEBUG_ONLY(locked_by_task));
      }
    }
  }

  void unlock_set_layout_binding(descriptor::SetLayoutBinding set_layout_binding COMMA_CWDEBUG_ONLY(AIStatefulTask* task)) /*thread-safe*/ const
  {
    DoutEntering(dc::notice, "unlock_set_layout_binding(" << set_layout_binding << ", " << task << ")");
    set_layout_bindings_to_handles_t::wat set_layout_bindings_to_handles_w(m_set_layout_bindings_to_handles);
    auto iter = set_layout_bindings_to_handles_w->find(set_layout_binding);
    // Call lock_set_layout_binding before calling unlock_set_layout_binding.
    ASSERT(iter != set_layout_bindings_to_handles_w->end());
    iter->second.unlock_descriptor_sets(CWDEBUG_ONLY(task));
  }

  void set_created()
  {
    m_created.store(true, std::memory_order::release);
  }

  void release_create_lock() /*thread-safe*/ const
  {
    DoutEntering(dc::notice, "Base::release_create_lock() [" << this << "]");
    m_create_access_mutex.unlock();
  }

  bool is_created() const
  {
    return m_created.load(std::memory_order::acquire);
  }

  virtual void instantiate(task::SynchronousWindow const* owning_window
      COMMA_CWDEBUG_ONLY(Ambifix const& ambifix)) = 0;
  virtual bool is_frame_resource() const { return false; }
  virtual void update_descriptor_set(task::SynchronousWindow const* owning_window, descriptor::FrameResourceCapableDescriptorSet const& descriptor_set, uint32_t binding, bool has_frame_resource) const = 0;
  virtual void prepare_shader_resource_declaration(descriptor::SetIndexHint set_index_hint, pipeline::ShaderInputData* shader_input_data) const = 0;
  //---------------------------------------------------------------------------

  // Accessors.
  descriptor::SetKey descriptor_set_key() const { return m_descriptor_set_key; }
  set_layout_bindings_to_handles_t::crat set_layout_bindings_to_handles() const { return m_set_layout_bindings_to_handles; }

#ifdef CWDEBUG
 public:
  virtual void print_on(std::ostream& os) const = 0;
#endif
};

} // namespace shader_resource
} // namespace vulkan::shader_builder
