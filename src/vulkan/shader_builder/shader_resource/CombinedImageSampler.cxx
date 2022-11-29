#include "sys.h"
#include "CombinedImageSampler.h"
#ifdef CWDEBUG
#include "vk_utils/print_pointer.h"
#endif

namespace vulkan::shader_builder::shader_resource {

void CombinedImageSampler::set_glsl_id_postfix(char const* glsl_id_full_postfix)
{
  // Only call set_glsl_id_postfix once, for example from create_texures.
  ASSERT(!m_descriptor_task);
  // Create the task that will realize the descriptor.
  m_descriptor_task = statefultask::create<descriptor::CombinedImageSampler>(glsl_id_full_postfix);
  m_descriptor_task->run();
}

void CombinedImageSampler::set_bindings_flags(vk::DescriptorBindingFlagBits binding_flags)
{
  m_descriptor_task->set_bindings_flags(binding_flags);
}

void CombinedImageSampler::set_array_size(uint32_t array_size, ArrayType array_type)
{
  m_descriptor_task->set_array_size(array_size, array_type);
}

#ifdef CWDEBUG
void CombinedImageSampler::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_descriptor_task:" << vk_utils::print_pointer(m_descriptor_task.get());
  os << '}';
}
#endif

} // namespace vulkan::shader_builder::shader_resource
