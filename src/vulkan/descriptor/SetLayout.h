#pragma once

#include "LayoutBindingCompare.h"
#include "shaderbuilder/ShaderResourceDeclarationContext.h"
#include "utils/VectorCompare.h"
#include "utils/sorted_vector_insert.h"
#include <vulkan/vulkan.hpp>
#include <vector>
#include <algorithm>
#include "debug.h"

namespace vulkan {
class LogicalDevice;
} // namespace vulkan

namespace vulkan::descriptor {

struct SetLayoutCompare;

class SetLayout
{
 private:
  friend struct SetLayoutCompare;
  std::vector<vk::DescriptorSetLayoutBinding> m_sorted_bindings;                                // The bindings "key" from which m_handle was created.
  vk::DescriptorSetLayout m_handle;

 public:
  SetLayout() = default;

#if 0
  SetLayout(std::vector<vk::DescriptorSetLayoutBinding>&& sorted_bindings, vk::DescriptorSetLayout handle) :
    m_sorted_bindings(std::move(sorted_bindings)), m_handle(handle) { }
#endif

  void push_back(vk::DescriptorSetLayoutBinding const& descriptor_set_layout_binding)
  {
    utils::sorted_vector_insert(m_sorted_bindings, descriptor_set_layout_binding, LayoutBindingCompare{});
  }

  // Look up m_sorted_bindings in cache, and create a new handle if it doesn't already exist.
  void realize_handle(LogicalDevice const* logical_device);

  vk::DescriptorSetLayout handle() const
  {
    return m_handle;
  }

  // Called from LogicalDevice::try_emplace_pipeline_layout.
  explicit operator vk::DescriptorSetLayout() const
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
    // Put default constructed SetLayout's at the end.
    if (lhs.m_sorted_bindings.empty())
      return false;
    if (rhs.m_sorted_bindings.empty())
      return true;

    return utils::VectorCompare<LayoutBindingCompare>{}(lhs.m_sorted_bindings, rhs.m_sorted_bindings);
  }
};

} // namespace vulkan::descriptor
