#pragma once

#include "ShaderResourceMember.h"
#include "Update.h"
#include "SetKeyContext.h"
#include "shader_resource/Base.h"
#include "pipeline/FactoryCharacteristicKey.h"
#include "pipeline/FactoryCharacteristicData.h"
#include "vk_utils/TaskToTaskDeque.h"
#include "cwds/debug_ostream_operators.h"
#include <boost/container/flat_map.hpp>

namespace vulkan::shader_builder::shader_resource {
class Texture;
} // namespace vulkan::shader_builder::shader_resource

namespace vulkan::descriptor {
using utils::has_print_on::operator<<;
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
class CombinedImageSampler : public vk_utils::TaskToTaskDeque<AIStatefulTask, boost::intrusive_ptr<Update>>, public shader_resource::Base
{
 protected:
  using direct_base_type = AIStatefulTask;

  enum combined_image_sampler_state_type {
    CombinedImageSampler_need_action = direct_base_type::state_end,
    CombinedImageSampler_done
  };

 public:
  static state_type constexpr state_end = CombinedImageSampler_done + 1;

 private:
  task::SynchronousWindow const* m_owning_window{};                             // The owning window.
  std::unique_ptr<detail::CombinedImageSamplerShaderResourceMember> m_member;   // A CombinedImageSampler only has a single "member".
  std::atomic<vk::DescriptorBindingFlags> m_binding_flags{};                    // Optional binding flags to use for this descriptor.
  std::atomic<uint32_t> m_array_size{1};                                        // Array size or one if this is not an array.
  using factory_characteristic_key_to_descriptor_t = std::vector<std::pair<pipeline::FactoryCharacteristicKey, pipeline::FactoryCharacteristicData>>;
  factory_characteristic_key_to_descriptor_t m_factory_characteristic_key_to_descriptor;        // The descriptor set / bindings associated with this CombinedImageSampler.
  using factory_characteristic_key_to_texture_t = std::vector<std::pair<pipeline::FactoryCharacteristicKey, shader_builder::shader_resource::Texture const*>>;
  factory_characteristic_key_to_texture_t m_factory_characteristic_key_to_texture;

 private:
  std::pair<factory_characteristic_key_to_descriptor_t::const_iterator, factory_characteristic_key_to_descriptor_t::const_iterator> find_descriptors(pipeline::FactoryCharacteristicKey const& key) const;
  factory_characteristic_key_to_texture_t::const_iterator find_texture(pipeline::FactoryCharacteristicKey const& key) const;

 public:
  CombinedImageSampler(char const* glsl_id_full_postfix COMMA_CWDEBUG_ONLY(bool debug = false)) :
    vk_utils::TaskToTaskDeque<AIStatefulTask, boost::intrusive_ptr<Update>>(CWDEBUG_ONLY(debug)), shader_resource::Base(SetKeyContext::instance(), glsl_id_full_postfix)
  {
    DoutEntering(dc::statefultask(mSMDebug)|dc::vulkan, "descriptor::CombinedImageSampler::CombinedImageSampler(" << debug::print_string(glsl_id_full_postfix) << " [" << this << "]");
    std::string glsl_id_full("CombinedImageSampler::");
    glsl_id_full.append(glsl_id_full_postfix);
    m_member = detail::CombinedImageSamplerShaderResourceMember::create(glsl_id_full);
  }

  // Probably not used.
  CombinedImageSampler(CombinedImageSampler&&) = default;

  void set_bindings_flags(vk::DescriptorBindingFlags binding_flags)
  {
    DoutEntering(dc::vulkan, "CombinedImageSampler::set_bindings_flags(" << binding_flags << ") [" << this << "]");
    // Don't call set_bindings_flags unless you have something to set :p.
    ASSERT(!!binding_flags);
    [[maybe_unused]] vk::DescriptorBindingFlags prev_binding_flags = m_binding_flags.exchange(binding_flags, std::memory_order::relaxed);
    // Don't call set_bindings_flags twice (with different values).
    ASSERT(!prev_binding_flags || prev_binding_flags == binding_flags);
  }

  void set_array_size(uint32_t array_size)
  {
    DoutEntering(dc::vulkan, "CombinedImageSampler::set_array_size(" << array_size << ") [" << this << "]");
    // Don't call set_array_size unless it's an array :p.
    ASSERT(array_size > 1);
    [[maybe_unused]] uint32_t prev_array_size = m_array_size.exchange(array_size, std::memory_order::relaxed);
    // Don't call set_array_size twice (with different values).
    ASSERT(prev_array_size == 1 || prev_array_size == array_size);
  }

  CombinedImageSampler& operator=(CombinedImageSampler&& rhs)
  {
    this->Base::operator=(std::move(rhs));
    m_member = std::move(rhs.m_member);
    m_array_size.store(rhs.m_array_size.load(std::memory_order::relaxed), std::memory_order::relaxed);
    return *this;
  }

  char const* glsl_id_full() const { return m_member->member().glsl_id_full(); }
  ShaderResourceMember const& member() const { return m_member->member(); }

  // There is no need to instantiate anything for CombinedImageSamplers.
  void instantiate(task::SynchronousWindow const* owning_window COMMA_CWDEBUG_ONLY(Ambifix const& ambifix)) override { }

  void prepare_shader_resource_declaration(SetIndexHint set_index_hint, pipeline::ShaderInputData* shader_input_data) const override final;

  void update_descriptor_set(DescriptorUpdateInfo descriptor_update_info) override final
  {
    DoutEntering(dc::shaderresource, "CombinedImageSampler::update_descriptor_set(" << descriptor_update_info << ")");

    boost::intrusive_ptr<Update> update = new DescriptorUpdateInfo(std::move(descriptor_update_info));

    // Pass new descriptors that need to be updated to this task (this is called from a PipelineFactory).
    have_new_datum(std::move(update));
  }

  vk::DescriptorBindingFlags binding_flags() const override final { return m_binding_flags; }
  uint32_t array_size() const override final { return m_array_size; }

 protected:
  ~CombinedImageSampler() override;
  char const* task_name_impl() const override { return "CombinedImageSampler"; }
  char const* state_str_impl(state_type run_state) const override;
  void multiplex_impl(state_type run_state) override;
//  void initialize_impl() override;
//  void abort_impl() override;
//  void finish_impl() override;

#ifdef CWDEBUG
 public:
  void print_on(std::ostream& os) const override;
#endif
};

} // namespace vulkan::descriptor
