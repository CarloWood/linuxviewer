#pragma once

#include "Update.h"
#include "pipeline/FactoryCharacteristicKey.h"
#include "pipeline/FactoryCharacteristicData.h"
#include <cstdint>
#ifdef CWDEBUG
#include <iosfwd>
#endif

namespace vulkan::task {
class SynchronousWindow;
} // namespace vulkan::task

namespace vulkan::descriptor {
class FrameResourceCapableDescriptorSet;

class DescriptorUpdateInfo : public Update
{
 private:
  task::SynchronousWindow const* m_owning_window;
  pipeline::FactoryCharacteristicId m_factory_characteristic_id;// The pipeline factory / characteristic range pair that created this descriptor.
  int m_fill_index;                                             // A range value with which this descriptor is used; -1 if created from initialize (which means the full range).
  int32_t m_descriptor_array_size;                              // 1 if this is not an array. Negative if unbounded.
  FrameResourceCapableDescriptorSet const* m_descriptor_set;    // The descriptor set that needs updating.
  uint32_t m_binding;                                           // The binding number that needs updating.
  //FIXME: is this needed? m_descriptor_set->is_frame_resource returns the same.
  bool m_has_frame_resource;                                    // True if this descriptor set is a frame resource.

 public:
  // Construct an uninitialized DescriptorUpdateInfo (required for use in a TaskToTaskDeque - we'll move-assign to it shortly after).
  DescriptorUpdateInfo() { }
  ~DescriptorUpdateInfo();

  DescriptorUpdateInfo(
      task::SynchronousWindow const* owning_window,
      pipeline::FactoryCharacteristicId const& factory_characteristic_id, int fill_index, int32_t descriptor_array_size,
      FrameResourceCapableDescriptorSet const& descriptor_set, uint32_t binding, bool has_frame_resource);

  DescriptorUpdateInfo(DescriptorUpdateInfo&& rhs) :
    m_owning_window(rhs.m_owning_window),
    m_factory_characteristic_id(rhs.m_factory_characteristic_id), m_fill_index(rhs.m_fill_index), m_descriptor_array_size(rhs.m_descriptor_array_size),
    m_descriptor_set(rhs.m_descriptor_set), m_binding(rhs.m_binding), m_has_frame_resource(rhs.m_has_frame_resource)
  {
    // Make sure the destructor won't call PipelineFactory::descriptor_set_updated.
    rhs.m_factory_characteristic_id = {};
  }

  // Accessors.
  task::SynchronousWindow const* owning_window() const { return m_owning_window; }
  pipeline::FactoryCharacteristicId const& factory_characteristic_id() const { return m_factory_characteristic_id; }
  pipeline::FactoryCharacteristicData data() const { return {*m_descriptor_set, m_binding}; }
  int fill_index() const { return m_fill_index; }
  int32_t descriptor_array_size() const { return m_descriptor_array_size; }
  FrameResourceCapableDescriptorSet const& descriptor_set() const { return *m_descriptor_set; }
  uint32_t binding() const { return m_binding; }
  bool has_frame_resource() const { return m_has_frame_resource; }

  pipeline::FactoryCharacteristicKey full_range_key() const { return {m_factory_characteristic_id, m_factory_characteristic_id.full_range()}; }
  pipeline::FactoryCharacteristicKey key() const
  {
    return m_fill_index == -1 ? full_range_key() : pipeline::FactoryCharacteristicKey{m_factory_characteristic_id, m_fill_index};
  }

  DescriptorUpdateInfo& operator=(DescriptorUpdateInfo&& rhs)
  {
    m_owning_window = rhs.m_owning_window;
    m_factory_characteristic_id = rhs.m_factory_characteristic_id;
    m_fill_index = rhs.m_fill_index;
    m_descriptor_array_size = rhs.m_descriptor_array_size;
    m_descriptor_set = rhs.m_descriptor_set;
    m_binding = rhs.m_binding;
    m_has_frame_resource = rhs.m_has_frame_resource;
    // Make sure the destructor won't call PipelineFactory::descriptor_set_updated.
    rhs.m_factory_characteristic_id = {};
    return *this;
  }

  bool is_descriptor_update_info() const override final
  {
    return true;
  }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::descriptor
