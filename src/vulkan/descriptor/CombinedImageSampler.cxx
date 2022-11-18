#include "sys.h"
#include "CombinedImageSampler.h"
#include "pipeline/ShaderInputData.h"
#ifdef CWDEBUG
#include "vk_utils/print_pointer.h"
#include "utils/has_print_on.h"
#endif

namespace vulkan::descriptor {

void CombinedImageSampler::prepare_shader_resource_declaration(descriptor::SetIndexHint set_index_hint, pipeline::ShaderInputData* shader_input_data) const
{
  shader_input_data->prepare_texture_declaration(*this, set_index_hint);
}

void CombinedImageSampler::update_descriptor_set(task::SynchronousWindow const* owning_window, descriptor::FrameResourceCapableDescriptorSet const& descriptor_set, uint32_t binding, bool CWDEBUG_ONLY(has_frame_resource)) const
{
  //FIXME: do not ignore this call.
  DoutEntering(dc::shaderresource, "CombinedImageSampler::update_descriptor_set(" << owning_window << ", " << descriptor_set << ", " << binding << ", " << std::boolalpha << has_frame_resource << ")");
}

#ifdef CWDEBUG
namespace detail {
using utils::has_print_on::operator<<;

void CombinedImageSamplerShaderResourceMember::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_member:" << m_member;
  os << '}';
}

} // namespace detail

void CombinedImageSampler::print_on(std::ostream& os) const
{
  os << '{';
  os << "(Base)";
  Base::print_on(os);
  os << ", m_member:" << vk_utils::print_pointer(m_member);
  os << '}';
}

#endif // CWDEBUG

} // namespace vulkan::descriptor
