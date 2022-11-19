#pragma once

#include <cstdint>
#ifdef CWDEBUG
#include <iosfwd>
#endif

namespace task {
class SynchronousWindow;
} // namespace task

namespace vulkan::descriptor {
class FrameResourceCapableDescriptorSet;

class NeedsUpdate
{
 private:
  task::SynchronousWindow const* m_owning_window;
  FrameResourceCapableDescriptorSet const* m_descriptor_set;
  uint32_t m_binding;
  bool m_has_frame_resource;

 public:
  // Construct an uninitialized NeedsUpdate (required for use in a TaskToTaskDeque - we'll move-assign to it shortly after).
  NeedsUpdate() { }

  NeedsUpdate(task::SynchronousWindow const* owning_window, FrameResourceCapableDescriptorSet const& descriptor_set, uint32_t binding, bool has_frame_resource) :
    m_owning_window(owning_window), m_descriptor_set(&descriptor_set), m_binding(binding), m_has_frame_resource(has_frame_resource) { }

  NeedsUpdate(NeedsUpdate&& rhs) : m_owning_window(rhs.m_owning_window), m_descriptor_set(rhs.m_descriptor_set), m_binding(rhs.m_binding), m_has_frame_resource(rhs.m_has_frame_resource) { }

  // Accessors.
  task::SynchronousWindow const* owning_window() const { return m_owning_window; }
  FrameResourceCapableDescriptorSet const& descriptor_set() const { return *m_descriptor_set; }
  uint32_t binding() const { return m_binding; }
  bool has_frame_resource() const { return m_has_frame_resource; }

  NeedsUpdate& operator=(NeedsUpdate&& rhs)
  {
    m_owning_window = rhs.m_owning_window;
    m_descriptor_set = rhs.m_descriptor_set;
    m_binding = rhs.m_binding;
    m_has_frame_resource = rhs.m_has_frame_resource;
    return *this;
  }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::descriptor
