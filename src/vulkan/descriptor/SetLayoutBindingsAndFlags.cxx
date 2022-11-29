#include "sys.h"
#include "SetLayoutBindingsAndFlags.h"
#include "debug/debug_ostream_operators.h"
#ifdef CWDEBUG
#include "vk_utils/print_flags.h"
#endif
#include "debug.h"

namespace vulkan::descriptor {

void SetLayoutBindingsAndFlags::insert(vk::DescriptorSetLayoutBinding const& descriptor_set_layout_binding, vk::DescriptorBindingFlags binding_flags, int32_t descriptor_array_size)
{
  // Case
  // 1. m_sorted_bindings = {}
  // 2. m_sorted_bindings = { a, b, c }
  //
  // a. binding_flags == 0
  // b. binding_flags != 0
  //
  // i. m_binding_flags.empty()
  // ii. !m_binding_flags.empty()
  //
  // Actions:
  // A. Do nothing.
  // B. Insert binding_flags into m_binding_flags.
  // C. Resize m_binding_flags to len.
  //
  // 1ai   A
  // 1aii  -
  // 1bi   B
  // 1bii  -
  // 2ai   A
  // 2aii  B
  // 2bi   CB
  // 2bii  B
  //
  size_t len = m_sorted_bindings.size();
  auto inserted_element = utils::sorted_vector_insert(m_sorted_bindings, descriptor_set_layout_binding, LayoutBindingCompare{});
  int inserted_index = inserted_element - m_sorted_bindings.begin();
  if (len > 0 && binding_flags && m_binding_flags.empty())      // 2bi
  {
    m_binding_flags.reserve(len + 1);
    m_binding_flags.resize(len);                                // Fill m_binding_flags with len zeroes.
  }
  if (binding_flags || !m_binding_flags.empty())                // If not ai do B.
  {
    if ((binding_flags & vk::DescriptorBindingFlagBits::eUpdateAfterBind))
      m_descriptor_set_layout_flags |= vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool;
    // Insert binding_flags into m_binding_flags.
    m_binding_flags.resize(len + 1);
    while (len > inserted_index)
    {
      m_binding_flags[len] = m_binding_flags[len - 1];
      --len;
    }
    m_binding_flags[inserted_index] = binding_flags;
  }
  if (descriptor_array_size < 0)
  {
    // Only one descriptor per descriptor set can be an unbounded array.
    ASSERT(m_unbounded_descriptor_array_size == 0);
    m_unbounded_descriptor_array_size = static_cast<uint32_t>(-descriptor_array_size);
  }
}

#ifdef CWDEBUG
void SetLayoutBindingsAndFlags::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_sorted_bindings:" << m_sorted_bindings <<
      ", m_binding_flags:" << m_binding_flags <<
      ", m_descriptor_set_layout_flags:" << m_descriptor_set_layout_flags <<
      ", m_unbounded_descriptor_array_size:" << m_unbounded_descriptor_array_size;
  os << '}';
}
#endif

} // namespace vulkan::descriptor
