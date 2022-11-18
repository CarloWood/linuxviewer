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
  shader_input_data->prepare_combined_image_sampler_declaration(*this, set_index_hint);
}

void CombinedImageSampler::update_descriptor_set(task::SynchronousWindow const* owning_window, descriptor::FrameResourceCapableDescriptorSet const& descriptor_set, uint32_t binding, bool CWDEBUG_ONLY(has_frame_resource)) const
{
  //FIXME: do not ignore this call.
  DoutEntering(dc::shaderresource, "CombinedImageSampler::update_descriptor_set(" << owning_window << ", " << descriptor_set << ", " << binding << ", " << std::boolalpha << has_frame_resource << ")");
}

CombinedImageSampler::~CombinedImageSampler()
{
  DoutEntering(dc::statefultask(mSMDebug)|dc::vulkan, "CombinedImageSampler::~CombinedImageSampler() [" << this << "]");
}

char const* CombinedImageSampler::state_str_impl(state_type run_state) const
{
  switch (run_state)
  {
    AI_CASE_RETURN(CombinedImageSampler_start);
    AI_CASE_RETURN(CombinedImageSampler_done);
  }
  AI_NEVER_REACHED
}

void CombinedImageSampler::multiplex_impl(state_type run_state)
{
  switch (run_state)
  {
    case CombinedImageSampler_start:
      break;
    case CombinedImageSampler_done:
      finish();
      break;
  }
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
