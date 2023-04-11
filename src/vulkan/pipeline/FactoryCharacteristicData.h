#pragma once

#include "../descriptor/FrameResourceCapableDescriptorSet.h"

namespace vulkan::pipeline {

class FactoryCharacteristicData
{
 private:
  descriptor::FrameResourceCapableDescriptorSet m_descriptor_set;
  uint32_t m_binding;

 public:
  FactoryCharacteristicData() = default;
  FactoryCharacteristicData(descriptor::FrameResourceCapableDescriptorSet const& descriptor_set, uint32_t binding) :
    m_descriptor_set(descriptor_set), m_binding(binding) { }

  // Accessors.
  descriptor::FrameResourceCapableDescriptorSet const& descriptor_set() const { return m_descriptor_set; }
  uint32_t binding() const { return m_binding; }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::pipeline
