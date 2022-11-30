#include "sys.h"
#include "ShaderResourceBase.h"
#ifdef CWDEBUG
#include <iomanip>
#endif
#include "debug.h"

namespace vulkan::shader_builder {

#ifdef CWDEBUG
using NAMESPACE_DEBUG::print_string;

bool SetMutexAndSetHandles::lock_descriptor_sets(AIStatefulTask* task, AIStatefulTask::condition_type condition)
{
  DoutEntering(dc::notice, "SetMutexAndSetHandles::lock_descriptor_sets(" << task << ", " << task->print_conditions(condition) << ") [" << this << "] (owner:" << print_string(m_debug_owner->debug_name()) << ")");
  bool success = m_mutex.lock(task, condition);
  if (success)
  {
    ASSERT(!m_debug_have_lock);
    m_debug_have_lock = task;
  }
  return success;
}

void SetMutexAndSetHandles::set_have_lock(AIStatefulTask* task)
{
  ASSERT(!m_debug_have_lock);
  m_debug_have_lock = task;
}

void SetMutexAndSetHandles::unlock_descriptor_sets(AIStatefulTask const* task)
{
  DoutEntering(dc::notice, "SetMutexAndSetHandles::unlock_descriptor_sets(" << task << ") [" << this << "] (owner:" << print_string(m_debug_owner->debug_name()) << ")");
  // Only the task that owns the lock can unlock it.
  if (m_debug_have_lock != task)
  {
    Dout(dc::notice, "m_debug_have_lock (" << m_debug_have_lock << ") != task (" << task << ")");
    Debug(attach_gdb());
  }
  ASSERT(m_debug_have_lock == task);
  m_debug_have_lock = nullptr;
  m_mutex.unlock();
}

void SetMutexAndSetHandles::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_descriptor_sets:" << m_descriptor_sets <<
      ", m_debug_have_lock:" << m_debug_have_lock;
  os << '}';
}

void ShaderResourceBase::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_created:" << std::boolalpha << m_created.load(std::memory_order::relaxed) <<
      ", m_descriptor_set_key:" << m_descriptor_set_key <<
      ", m_set_layout_bindings:";
  os << *set_layout_bindings_to_handles_t::crat{m_set_layout_bindings_to_handles};
  os << '}';
}
#endif

} // namespace vulkan::shader_builder
