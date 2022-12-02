#pragma once

#include "LayoutBindingCompare.h"
#include "utils/sorted_vector_insert.h"
#include "debug.h"

namespace vulkan::descriptor {

class SetLayoutBindingsAndFlags
{
 private:
  std::vector<vk::DescriptorSetLayoutBinding> m_sorted_bindings;
  std::vector<vk::DescriptorBindingFlags> m_binding_flags;
  vk::DescriptorSetLayoutCreateFlags m_descriptor_set_layout_flags{};
  uint32_t m_unbounded_descriptor_array_size{};

 public:
  SetLayoutBindingsAndFlags() = default;
  SetLayoutBindingsAndFlags(std::vector<vk::DescriptorSetLayoutBinding>&& sorted_bindings) : m_sorted_bindings(std::move(sorted_bindings)) { }

  SetLayoutBindingsAndFlags(std::vector<vk::DescriptorSetLayoutBinding>&& sorted_bindings, std::vector<vk::DescriptorBindingFlags>&& binding_flags) : m_sorted_bindings(std::move(sorted_bindings)), m_binding_flags(std::move(binding_flags)) { }

  // Accessors.
  std::vector<vk::DescriptorSetLayoutBinding>& sorted_bindings() { return m_sorted_bindings; }
  std::vector<vk::DescriptorSetLayoutBinding> const& sorted_bindings() const { return m_sorted_bindings; }
  std::vector<vk::DescriptorBindingFlags> const& binding_flags() const { return m_binding_flags; }
  vk::DescriptorSetLayoutCreateFlags descriptor_set_layout_flags() const { return m_descriptor_set_layout_flags; }
  uint32_t unbounded_descriptor_array_size() const { return m_unbounded_descriptor_array_size; }

  void insert(vk::DescriptorSetLayoutBinding const& descriptor_set_layout_binding, vk::DescriptorBindingFlags binding_flags, int32_t descriptor_array_size);

  bool empty() const { return m_sorted_bindings.empty(); }
  size_t size() const { return m_sorted_bindings.size(); }

  friend bool operator==(SetLayoutBindingsAndFlags const& lhs, SetLayoutBindingsAndFlags const& rhs)
  {
    return lhs.m_sorted_bindings == rhs.m_sorted_bindings && lhs.m_binding_flags == rhs.m_binding_flags;
  }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::descriptor
