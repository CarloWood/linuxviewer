#pragma once

#include "FrameResourceIndex.h"
#include "utils/Vector.h"
#include <vulkan/vulkan.hpp>
#ifdef CWDEBUG
#include "debug/vulkan_print_on.h"
#endif
#include "debug.h"

namespace vulkan::descriptor {

class FrameResourceCapableDescriptorSetAsKey
{
 private:
  VkDescriptorSet m_key;

 public:
  FrameResourceCapableDescriptorSetAsKey(vk::DescriptorSet key) : m_key(key) { }

  // This class is used in a std::map as key.
  friend bool operator<(FrameResourceCapableDescriptorSetAsKey lhs, FrameResourceCapableDescriptorSetAsKey rhs)
  {
    return lhs.m_key < rhs.m_key;
  }

  // Also used by CombinedImageSampler to assure that all descriptor/binding pairs are in a single subrange of the CharacteristicRange.
  friend bool operator==(FrameResourceCapableDescriptorSetAsKey lhs, FrameResourceCapableDescriptorSetAsKey rhs)
  {
    return lhs.m_key == rhs.m_key;
  }
};

class FrameResourceCapableDescriptorSet
{
 private:
  // Depending on whether or not this is bound to a FrameResource capable shader resource,
  // this is either a single descriptor set handle, or multiple: one per frame resource.
  utils::Vector<vk::DescriptorSet, FrameResourceIndex> m_descriptor_set;

 public:
  FrameResourceCapableDescriptorSet() = default;
  FrameResourceCapableDescriptorSet(FrameResourceCapableDescriptorSet&& orig) :
    m_descriptor_set(std::move(orig.m_descriptor_set)) { }

  FrameResourceCapableDescriptorSet(FrameResourceCapableDescriptorSet const& orig) :
    m_descriptor_set(orig.m_descriptor_set) { }

  FrameResourceCapableDescriptorSet(std::vector<vk::DescriptorSet>::const_iterator begin, std::vector<vk::DescriptorSet>::const_iterator end) :
    m_descriptor_set(begin, end) { }

  FrameResourceCapableDescriptorSet(vk::DescriptorSet vh_descriptor_set) :
    m_descriptor_set(1, vh_descriptor_set) { }

  FrameResourceCapableDescriptorSet& operator=(FrameResourceCapableDescriptorSet const& rhs)
  {
    m_descriptor_set = rhs.m_descriptor_set;
    return *this;
  }

  bool is_used() const
  {
    return !m_descriptor_set.empty();
  }

  bool is_frame_resource() const
  {
    return m_descriptor_set.size() > 1;
  }

  operator vk::DescriptorSet() const
  {
    // Only use automatic conversion when this represents a single descriptor set handle.
    ASSERT(!is_frame_resource());
    return *m_descriptor_set.begin();
  }

  FrameResourceCapableDescriptorSetAsKey as_key() const
  {
    // Just returning the first descriptor set will do.
    return *m_descriptor_set.begin();
  }

  vk::DescriptorSet operator[](FrameResourceIndex index) const
  {
    // Only use operator[] when this represents more than one descriptor set handle; one per frame resource.
    ASSERT(is_frame_resource());
    return m_descriptor_set[index];
  }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::descriptor
