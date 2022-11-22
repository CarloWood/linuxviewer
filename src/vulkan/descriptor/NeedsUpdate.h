#pragma once

#include "pipeline/FactoryRangeId.h"
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
  pipeline::FactoryRangeId m_factory_range_id;                            // The pipeline factory / characteristic range pair that created this descriptor.
  int m_fill_index;                                             // A range value with which this descriptor is used.
  uint32_t m_array_size;                                        // 1 if this is not an array; 0 if no size is known.
  FrameResourceCapableDescriptorSet const* m_descriptor_set;    // The descriptor set that needs updating.
  uint32_t m_binding;                                           // The binding number that needs updating.
  //FIXME: is this needed? m_descriptor_set->is_frame_resource returns the same.
  bool m_has_frame_resource;                                    // True if this descriptor set is a frame resource.

 public:
  // Construct an uninitialized NeedsUpdate (required for use in a TaskToTaskDeque - we'll move-assign to it shortly after).
  NeedsUpdate() { }

  NeedsUpdate(
      task::SynchronousWindow const* owning_window,
      pipeline::FactoryRangeId const& factory_range_id, int fill_index, uint32_t array_size,
      FrameResourceCapableDescriptorSet const& descriptor_set, uint32_t binding, bool has_frame_resource) :
    m_owning_window(owning_window),
    m_factory_range_id(factory_range_id), m_fill_index(fill_index), m_array_size(array_size),
    m_descriptor_set(&descriptor_set), m_binding(binding), m_has_frame_resource(has_frame_resource) { }

  NeedsUpdate(NeedsUpdate&& rhs) :
    m_owning_window(rhs.m_owning_window),
    m_factory_range_id(rhs.m_factory_range_id), m_fill_index(rhs.m_fill_index), m_array_size(rhs.m_array_size),
    m_descriptor_set(rhs.m_descriptor_set), m_binding(rhs.m_binding), m_has_frame_resource(rhs.m_has_frame_resource) { }

  // Accessors.
  task::SynchronousWindow const* owning_window() const { return m_owning_window; }
  FrameResourceCapableDescriptorSet const& descriptor_set() const { return *m_descriptor_set; }
  uint32_t binding() const { return m_binding; }
  bool has_frame_resource() const { return m_has_frame_resource; }

  NeedsUpdate& operator=(NeedsUpdate&& rhs)
  {
    m_owning_window = rhs.m_owning_window;
    m_factory_range_id = rhs.m_factory_range_id;
    m_fill_index = rhs.m_fill_index;
    m_array_size = rhs.m_array_size;
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
