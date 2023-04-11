#pragma once

#include "../shader_builder/ShaderResourceDeclarationContext.h"
#include "../descriptor/SetLayoutBindingsAndFlags.h"
#include "utils/VectorCompare.h"
#include <vulkan/vulkan.hpp>
#include <vector>
#include <algorithm>
#ifdef CWDEBUG
#include "../shader_builder/ShaderResourceDeclaration.h"
#include "../debug/debug_ostream_operators.h"
#include "../vk_utils/print_flags.h"
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
  SetLayoutBindingsAndFlags m_sorted_bindings_and_flags;        // The bindings "key" from which m_handle was created.
  SetIndexHint m_set_index_hint;                                // The set index (hint) that this SetLayout refers to. It is not used for sorting (not part of the 'key').
  vk::DescriptorSetLayout m_handle;

 public:
  SetLayout(SetIndexHint set_index_hint) : m_set_index_hint(set_index_hint)
  {
    DoutEntering(dc::setindexhint, "SetLayout::SetLayout(" << set_index_hint << ") [" << this << "]");
  }

  void insert_descriptor_set_layout_binding(vk::DescriptorSetLayoutBinding const& descriptor_set_layout_binding, vk::DescriptorBindingFlags binding_flags, int32_t descriptor_array_size)
  {
    DoutEntering(dc::vulkan, "SetLayout::insert_descriptor_set_layout_binding(" << descriptor_set_layout_binding << ", " << binding_flags << ", " << descriptor_array_size << ") [" << this << "]");
    // Paranoia check: we shouldn't be trying to insert something with the same stage flags and binding.
    ASSERT(std::find_if(m_sorted_bindings_and_flags.sorted_bindings().begin(), m_sorted_bindings_and_flags.sorted_bindings().end(),
          [&](vk::DescriptorSetLayoutBinding const& element){
            return element.binding == descriptor_set_layout_binding.binding &&
                   (element.stageFlags & descriptor_set_layout_binding.stageFlags);
          }) == m_sorted_bindings_and_flags.sorted_bindings().end());
    m_sorted_bindings_and_flags.insert(descriptor_set_layout_binding, binding_flags, descriptor_array_size);
  }

  // Look up m_sorted_bindings_and_flags in cache, and create a new handle if it doesn't already exist.
  // Otherwise use the existing handle and fix the binding numbers in m_sorted_bindings_and_flags to match that layout.
  void realize_handle(LogicalDevice const* logical_device);

  // Accessors.
  SetLayoutBindingsAndFlags const& sorted_bindings_and_flags() const { return m_sorted_bindings_and_flags; }
  SetLayoutBindingsAndFlags& sorted_bindings_and_flags() { return m_sorted_bindings_and_flags; }
  SetIndexHint set_index_hint() const { return m_set_index_hint; }
  void set_set_index_hint(SetIndexHint set_index)
  {
    DoutEntering(dc::setindexhint, "SetLayout::set_set_index_hint(" << set_index << ") [" << this << "]");
    m_set_index_hint = set_index;
  }
  vk::DescriptorSetLayout handle() const { return m_handle; }

  // Called from LogicalDevice::realize_pipeline_layout.
  explicit operator vk::DescriptorSetLayout() const
  {
    return m_handle;
  }

#ifdef CWDEBUG
  friend bool operator==(SetLayout const& sl1, SetLayout const& sl2)
  {
    // Used to test if sl1 was indeed a copy of sl2.
    return sl1.m_handle == sl2.m_handle && sl1.m_set_index_hint == sl2.m_set_index_hint && sl1.m_sorted_bindings_and_flags == sl2.m_sorted_bindings_and_flags;
  }

  void print_on(std::ostream& os) const
  {
    os << '{';
    os << "m_sorted_bindings_and_flags:" << m_sorted_bindings_and_flags <<
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
    if (lhs.m_sorted_bindings_and_flags.empty())
      return false;
    if (rhs.m_sorted_bindings_and_flags.empty())
      return true;

    auto compare = lhs.m_sorted_bindings_and_flags.sorted_bindings() <=> rhs.m_sorted_bindings_and_flags.sorted_bindings();
    if (compare < 0)    // lhs < rhs
      return true;
    if (compare > 0)    // rhs < lhs
      return false;
    return (lhs.m_sorted_bindings_and_flags.binding_flags() <=> rhs.m_sorted_bindings_and_flags.binding_flags()) < 0;
  }
};

struct CompareHint
{
  SetIndexHint m_set_index_hint;
  bool operator()(SetLayout const& set_layout) { return set_layout.set_index_hint() == m_set_index_hint; }
};

} // namespace vulkan::descriptor
