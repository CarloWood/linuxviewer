#pragma once

#include "LayoutBindingCompare.h"
#include "utils/VectorCompare.h"
#include <vulkan/vulkan.hpp>
#include <vector>
#include "debug.h"

namespace vulkan::descriptor {

struct SetLayoutCompare;

class SetLayout
{
 private:
  friend struct SetLayoutCompare;
  std::vector<vk::DescriptorSetLayoutBinding> m_sorted_bindings;        // The bindings "key" from which m_handle was created.
  vk::DescriptorSetLayout m_handle;

 public:
  SetLayout() = default;

  SetLayout(std::vector<vk::DescriptorSetLayoutBinding>&& sorted_bindings, vk::DescriptorSetLayout handle) :
    m_sorted_bindings(std::move(sorted_bindings)), m_handle(handle) { }

  operator vk::DescriptorSetLayout() const
  {
    return m_handle;
  }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const
  {
    os << '{';
    os << "m_sorted_bindings:" << m_sorted_bindings <<
        ", m_handle:" << m_handle;
    os << '}';
  }
#endif
};

struct SetLayoutCompare
{
  bool operator()(SetLayout const& lhs, SetLayout const& rhs) const
  {
    return utils::VectorCompare<LayoutBindingCompare>{}(lhs.m_sorted_bindings, rhs.m_sorted_bindings);
  }
};

} // namespace vulkan::descriptor
