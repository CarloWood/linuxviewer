#pragma once

#include "LayoutBindingCompare.h"
#include "shader_builder/ShaderResourceDeclarationContext.h"
#include "utils/VectorCompare.h"
#include "utils/sorted_vector_insert.h"
#include <vulkan/vulkan.hpp>
#include <vector>
#include <algorithm>
#ifdef CWDEBUG
#include "debug/debug_ostream_operators.h"
#endif
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
  std::vector<vk::DescriptorSetLayoutBinding> m_sorted_bindings;        // The bindings "key" from which m_handle was created.
  SetIndexHint m_set_index_hint;                                        // The set index (hint) that this SetLayout refers to. It is not used for sorting (not part of the 'key').
  vk::DescriptorSetLayout m_handle;

 public:
  SetLayout(SetIndexHint set_index_hint) : m_set_index_hint(set_index_hint) { }

  void push_back(vk::DescriptorSetLayoutBinding const& descriptor_set_layout_binding)
  {
    DoutEntering(dc::vulkan, "SetLayout::push_back(" << descriptor_set_layout_binding << ")");
    utils::sorted_vector_insert(m_sorted_bindings, descriptor_set_layout_binding, LayoutBindingCompare{});
  }

  // Look up m_sorted_bindings in cache, and create a new handle if it doesn't already exist.
  void realize_handle(LogicalDevice const* logical_device);

  // Accessors.
  std::vector<vk::DescriptorSetLayoutBinding> const& sorted_bindings() const { return m_sorted_bindings; }
  SetIndexHint set_index_hint() const { return m_set_index_hint; }
  vk::DescriptorSetLayout handle() const { return m_handle; }

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
        ", m_set_index_hint:" << m_set_index_hint <<
        ", m_handle:" << m_handle;
    os << '}';
  }
#endif
};

struct SetLayoutCompare
{
  bool operator()(SetLayout const& lhs, SetLayout const& rhs) const
  {
    // Put just constructed SetLayout's at the end.
    if (lhs.m_sorted_bindings.empty())
      return false;
    if (rhs.m_sorted_bindings.empty())
      return true;

    return utils::VectorCompare<LayoutBindingCompare>{}(lhs.m_sorted_bindings, rhs.m_sorted_bindings);
#if 0
    if (utils::VectorCompare<LayoutBindingCompare>{}(lhs.m_sorted_bindings, rhs.m_sorted_bindings))
      return true;

    if (utils::VectorCompare<LayoutBindingCompare>{}(rhs.m_sorted_bindings, lhs.m_sorted_bindings))
      return false;

    return lhs.m_set_index_hint < rhs.m_set_index_hint;
#endif
  }
};

} // namespace vulkan::descriptor
