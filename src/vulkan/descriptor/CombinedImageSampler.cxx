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

void CombinedImageSampler::update_descriptor_set(NeedsUpdate descriptor_to_update)
{
  DoutEntering(dc::shaderresource, "CombinedImageSampler::update_descriptor_set(" << descriptor_to_update << ")");

  // Pass new descriptors that need to be updated to this task (this is called from a PipelineFactory).
  have_new_datum(std::move(descriptor_to_update));
}

CombinedImageSampler::~CombinedImageSampler()
{
  DoutEntering(dc::statefultask(mSMDebug)|dc::vulkan, "CombinedImageSampler::~CombinedImageSampler() [" << this << "]");
}

char const* CombinedImageSampler::state_str_impl(state_type run_state) const
{
  switch (run_state)
  {
    AI_CASE_RETURN(CombinedImageSampler_need_action);
    AI_CASE_RETURN(CombinedImageSampler_done);
  }
  AI_NEVER_REACHED
}

void CombinedImageSampler::multiplex_impl(state_type run_state)
{
  switch (run_state)
  {
    case CombinedImageSampler_need_action:
      flush_new_data([this](Datum&& datum){
          Dout(dc::always, "Received: " << datum);
        });
      if (producer_not_finished())
        break;
      set_state(CombinedImageSampler_done);
      [[fallthrough]];
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
