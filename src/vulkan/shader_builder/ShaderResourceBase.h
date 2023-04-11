#pragma once

#include "../FrameResourceIndex.h"
#include "../descriptor/FrameResourceCapableDescriptorSet.h"
#include "../descriptor/SetLayout.h"
#include "../descriptor/SetLayoutBinding.h"
#include "../descriptor/DescriptorUpdateInfo.h"
#include "../pipeline/ShaderResourcePlusCharacteristicIndex.h"
#include "threadsafe/aithreadsafe.h"
#include "threadsafe/AIReadWriteMutex.h"
#include <vulkan/vulkan.hpp>
#ifdef CWDEBUG
#include "../debug/DebugSetName.h"
#endif
#include "debug.h"

namespace vulkan::task {
class SynchronousWindow;
} // namespace vulkan::task

namespace vulkan::pipeline {
class AddShaderStage;
} // namespace vulkan::pipeline

namespace vulkan::shader_builder {
class ShaderResourceDeclaration;
class ShaderResourceBase;

// Helper class that stores the descriptor set allocation mutex and a vector with FrameResourceCapableDescriptorSet handles bound to
// the associated shader resource (m_debug_owner).
//
// Access to this class is protected by the read/write mutex on ShaderResourceBase::m_set_layout_bindings_to_handles.
class SetMutexAndSetHandles
{
 public:
  using descriptor_set_container_t = std::vector<descriptor::FrameResourceCapableDescriptorSet>;

 private:
  AIStatefulTaskMutex m_mutex;                  // Mutex that is locked for the first shader resource of a given set a shader resources that we need a descriptor for.

  descriptor_set_container_t m_descriptor_sets; // Vector containing all descriptor sets that the shader resource owning this SetLayoutBinding (stored in ShaderResourceBase::m_set_layout_bindings) is bound to.

#ifdef CWDEBUG
  AIStatefulTask const* m_debug_have_lock{};    // Pointer to the task that successfully locked m_mutex, or nullptr when unlocked.
  ShaderResourceBase const* m_debug_owner;      // The shader resource that has a map that contains this object.
#endif

 public:
#ifndef CWDEBUG
  // Construct a SetMutexAndSetHandles that contains no FrameResourceCapableDescriptorSet's.
  SetMutexAndSetHandles() = default;
  // Construct a SetMutexAndSetHandles that contains a single FrameResourceCapableDescriptorSet.
  SetMutexAndSetHandles(descriptor::FrameResourceCapableDescriptorSet const& descriptor_set) : m_descriptor_sets(1, descriptor_set) { }
#else
  // Construct a SetMutexAndSetHandles that contains no FrameResourceCapableDescriptorSet's.
  SetMutexAndSetHandles(ShaderResourceBase const* owner) : m_debug_owner(owner) { }
  // Construct a SetMutexAndSetHandles that contains a single FrameResourceCapableDescriptorSet.
  SetMutexAndSetHandles(descriptor::FrameResourceCapableDescriptorSet const& descriptor_set, ShaderResourceBase const* owner) :
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
  descriptor_set_container_t const& descriptor_sets() const { return m_descriptor_sets; }

#ifdef CWDEBUG
  void unlock_descriptor_sets(AIStatefulTask const* task);
#else
  void unlock_descriptor_sets() { m_mutex.unlock(); }
#endif

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

// ShaderResourceBase class for shader resources.
class ShaderResourceBase
{
 public:
  using set_layout_bindings_to_handles_container_t = std::map<descriptor::SetLayoutBinding, SetMutexAndSetHandles>;

 private:
  // 'mutable' because shader resource instances added with add_* (add_combined_image_sampler, add_uniform_buffer, etc)
  // will only be created ONCE - synchronizing PipelineFactory by m_create_access_mutex, so that only a single
  // PipelineFactory will create the shader resources while all other PipelineFactory's will be waiting for
  // that to be finished. The thread-safe functions that use this mutex aren't really 'const', but they
  // are thread-safe ;).
  mutable AIStatefulTaskMutex m_create_access_mutex;
  std::atomic_bool m_created{false};
  descriptor::SetKey const m_descriptor_set_key;
  using set_layout_bindings_to_handles_t = aithreadsafe::Wrapper<set_layout_bindings_to_handles_container_t, aithreadsafe::policy::ReadWrite<AIReadWriteMutex>>;
  mutable set_layout_bindings_to_handles_t m_set_layout_bindings_to_handles;

 protected:
#ifdef CWDEBUG
 private:
  char const* m_debug_name;

 public:
  char const* debug_name() const { return m_debug_name; }
#endif

 public:
  ShaderResourceBase(descriptor::SetKey descriptor_set_key COMMA_CWDEBUG_ONLY(char const* debug_name)) :
    m_descriptor_set_key(descriptor_set_key) COMMA_CWDEBUG_ONLY(m_debug_name(debug_name))
  {
    DoutEntering(dc::notice, "ShaderResourceBase::ShaderResourceBase(" << descriptor_set_key << ", " << NAMESPACE_DEBUG::print_string(debug_name) << ") [" << this << "]");
  }
  ShaderResourceBase() = default;     // Constructs a non-sensical m_descriptor_set_key that must be ignored when moving the object (in place).

  // Ignore m_descriptor_set_key when move- assigning and/or copying; the target should already have the right key.
  ShaderResourceBase(ShaderResourceBase&& rhs)
  {
  }
  ShaderResourceBase& operator=(ShaderResourceBase&& rhs)
  {
    return *this;
  }

  // Disallow copy and move constructing.
  ShaderResourceBase(ShaderResourceBase const& rhs) = delete;

  //---------------------------------------------------------------------------
  // The call to create and update_descriptor_set must happen after acquire_lock returned true,
  // followed by a call to set_created and before the accompanied call to release_lock.

  bool acquire_create_lock(AIStatefulTask* task, AIStatefulTask::condition_type condition) /*threadsafe-*/ const
  {
    DoutEntering(dc::notice, "ShaderResourceBase::acquire_create_lock(" << task << ", " << task->print_conditions(condition) << ") [" << this << "]");
    return m_create_access_mutex.lock(task, condition);
  }

  bool lock_set_layout_binding(descriptor::SetLayoutBinding set_layout_binding, AIStatefulTask* task, AIStatefulTask::condition_type condition) /*threadsafe-*/ const
  {
    DoutEntering(dc::notice, "lock_set_layout_binding(" << set_layout_binding << ", " << task << ", " << task->print_conditions(condition) << ")");
    set_layout_bindings_to_handles_t::wat set_layout_bindings_to_handles_w(m_set_layout_bindings_to_handles);
    auto ibp = set_layout_bindings_to_handles_w->try_emplace(set_layout_binding COMMA_CWDEBUG_ONLY(this));  // Get existing or default constructed SetMutexAndSetHandles.
    return ibp.first->second.lock_descriptor_sets(task, condition);
  }

#ifdef CWDEBUG
  void set_have_lock(descriptor::SetLayoutBinding set_layout_binding, AIStatefulTask* task) /*threadsafe-*/ const
  {
    DoutEntering(dc::notice, "set_have_lock(" << set_layout_binding << ", " << task << ")");
    set_layout_bindings_to_handles_t::wat set_layout_bindings_to_handles_w(m_set_layout_bindings_to_handles);
    auto iter = set_layout_bindings_to_handles_w->find(set_layout_binding);     // Get existing SetMutexAndSetHandles (see lock_set_layout_binding which created it when it didn't already exist then).
    ASSERT(iter != set_layout_bindings_to_handles_w->end());
    return iter->second.set_have_lock(task);
  }
#endif

  void add_set_layout_binding(descriptor::SetLayoutBinding set_layout_binding, descriptor::FrameResourceCapableDescriptorSet const& descriptor_set, AIStatefulTask* locked_by_task) /*threadsafe-*/ const
  {
    DoutEntering(dc::notice, "add_set_layout_binding(" << set_layout_binding << ", " << descriptor_set << ", " << locked_by_task << ") for [" << this << "]");
    set_layout_bindings_to_handles_t::wat set_layout_bindings_to_handles_w(m_set_layout_bindings_to_handles);
    Dout(dc::notice, "m_set_layout_bindings_to_handles currently contains: " << *set_layout_bindings_to_handles_w);
    auto iter = set_layout_bindings_to_handles_w->find(set_layout_binding);
    if (iter == set_layout_bindings_to_handles_w->end())
    {
      Dout(dc::notice, "The key was not found, adding it...");
      set_layout_bindings_to_handles_w->try_emplace(set_layout_binding, descriptor_set COMMA_CWDEBUG_ONLY(this));
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

  void unlock_set_layout_binding(descriptor::SetLayoutBinding set_layout_binding COMMA_CWDEBUG_ONLY(AIStatefulTask* task)) /*threadsafe-*/ const
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

  void release_create_lock() /*threadsafe-*/ const
  {
    DoutEntering(dc::notice, "ShaderResourceBase::release_create_lock() [" << this << "]");
    m_create_access_mutex.unlock();
  }

  bool is_created() const
  {
    return m_created.load(std::memory_order::acquire);
  }

  virtual void instantiate(task::SynchronousWindow const* owning_window
      COMMA_CWDEBUG_ONLY(Ambifix const& ambifix)) = 0;
  virtual bool is_frame_resource() const { return false; }
  virtual void update_descriptor_set(descriptor::DescriptorUpdateInfo descriptor_update_info) = 0;
  virtual void prepare_shader_resource_declaration(descriptor::SetIndexHint set_index_hint, pipeline::AddShaderStage* add_shader_stage) const = 0;
  virtual vk::DescriptorBindingFlags binding_flags() const { return {}; }
  virtual int32_t descriptor_array_size() const { return 1; }      // One means it's not an array. Negative means unbounded.

  //---------------------------------------------------------------------------

  // Accessors.
  descriptor::SetKey descriptor_set_key() const { return m_descriptor_set_key; }
  set_layout_bindings_to_handles_t::crat set_layout_bindings_to_handles() const { return m_set_layout_bindings_to_handles; }

#ifdef CWDEBUG
 public:
  virtual void print_on(std::ostream& os) const = 0;
#endif
};

} // namespace vulkan::shader_builder
