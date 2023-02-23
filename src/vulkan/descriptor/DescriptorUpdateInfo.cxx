#include "sys.h"
#include "DescriptorUpdateInfo.h"
#include "FrameResourceCapableDescriptorSet.h"
#include "SynchronousWindow.h"
#include "debug.h"

namespace vulkan::descriptor {

DescriptorUpdateInfo::DescriptorUpdateInfo(
    task::SynchronousWindow const* owning_window,
    pipeline::FactoryCharacteristicId const& factory_characteristic_id, int fill_index, int32_t descriptor_array_size,
    FrameResourceCapableDescriptorSet const& descriptor_set, uint32_t binding, bool has_frame_resource) :
  m_owning_window(owning_window),
  m_factory_characteristic_id(factory_characteristic_id), m_fill_index(fill_index), m_descriptor_array_size(descriptor_array_size),
  m_descriptor_set(&descriptor_set), m_binding(binding), m_has_frame_resource(has_frame_resource)
{
  auto factory_index = m_factory_characteristic_id.factory_index();
  m_owning_window->pipeline_factory(factory_index)->descriptor_set_update_start();
}

DescriptorUpdateInfo::~DescriptorUpdateInfo()
{
  auto factory_index = m_factory_characteristic_id.factory_index();
  if (!factory_index.undefined())
    m_owning_window->pipeline_factory(factory_index)->descriptor_set_updated();
}

#ifdef CWDEBUG
void DescriptorUpdateInfo::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_owning_window:" << m_owning_window <<
      ", m_factory_characteristic_id:" << m_factory_characteristic_id <<
      ", m_fill_index:" << m_fill_index <<
      ", m_descriptor_array_size:" << m_descriptor_array_size <<
      ", m_descriptor_set:" << m_descriptor_set <<
      ", m_binding:" << m_binding <<
      ", m_has_frame_resource:" << m_has_frame_resource;
  os << '}';
}
#endif

} // namespace vulkan::descriptor
