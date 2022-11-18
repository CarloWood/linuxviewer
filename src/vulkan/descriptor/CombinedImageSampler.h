#pragma once

#include "shader_resource/Base.h"
#include "ShaderResourceMember.h"
#include "descriptor/SetKeyContext.h"
#include "cwds/debug_ostream_operators.h"

namespace vulkan::descriptor {
using namespace shader_builder;

namespace detail {

// A wrapper around ShaderResourceMember, which doesn't have
// a default constructor, together with the run-time constructed
// glsl id string, so that below we only need a single memory
// allocation.
class CombinedImageSamplerShaderResourceMember
{
 private:
  ShaderResourceMember m_member;
  char m_glsl_id_full[];

  CombinedImageSamplerShaderResourceMember(std::string const& glsl_id_full) : m_member(m_glsl_id_full)
  {
    strcpy(m_glsl_id_full, glsl_id_full.data());
  }

 public:
  static std::unique_ptr<CombinedImageSamplerShaderResourceMember> create(std::string const& glsl_id_full)
  {
    void* memory = malloc(sizeof(ShaderResourceMember) + glsl_id_full.size() + 1);
    return std::unique_ptr<CombinedImageSamplerShaderResourceMember>{new (memory) CombinedImageSamplerShaderResourceMember(glsl_id_full)};
  }

  void operator delete(void* p)
  {
    free(p);
  }

  // Accessor.
  ShaderResourceMember const& member() const { return m_member; }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace detail

// Data collection used for textures.
class CombinedImageSampler : public shader_resource::Base
{
 private:
  std::unique_ptr<detail::CombinedImageSamplerShaderResourceMember> m_member;        // A CombinedImageSampler only has a single "member".

 public:
  CombinedImageSampler(char const* glsl_id_full_postfix) : shader_resource::Base(SetKeyContext::instance() COMMA_CWDEBUG_ONLY(glsl_id_full_postfix))
  {
    DoutEntering(dc::vulkan, "descriptor::CombinedImageSampler::CombinedImageSampler(" << debug::print_string(glsl_id_full_postfix) << " [" << this << "]");
    std::string glsl_id_full("CombinedImageSampler::");
    glsl_id_full.append(glsl_id_full_postfix);
    m_member = detail::CombinedImageSamplerShaderResourceMember::create(glsl_id_full);
  }

  CombinedImageSampler(CombinedImageSampler&& rhs) : shader_resource::Base(std::move(rhs)), m_member(std::move(rhs.m_member)) { }

  CombinedImageSampler& operator=(CombinedImageSampler&& rhs)
  {
    this->Base::operator=(std::move(rhs));
    m_member = std::move(rhs.m_member);
    return *this;
  }

  char const* glsl_id_full() const { return m_member->member().glsl_id_full(); }
  ShaderResourceMember const& member() const { return m_member->member(); }

  // There is no need to instantiate anything for CombinedImageSamplers.
  void instantiate(task::SynchronousWindow const* owning_window COMMA_CWDEBUG_ONLY(Ambifix const& ambifix)) override { }

  void prepare_shader_resource_declaration(descriptor::SetIndexHint set_index_hint, pipeline::ShaderInputData* shader_input_data) const override;

  void update_descriptor_set(task::SynchronousWindow const* owning_window, descriptor::FrameResourceCapableDescriptorSet const& descriptor_set, uint32_t binding, bool has_frame_resource) const override;

#ifdef CWDEBUG
  void print_on(std::ostream& os) const override;
#endif
};

} // namespace vulkan::descriptor
