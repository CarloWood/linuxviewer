#pragma once

#include "ShaderResourceMember.h"
#include "Update.h"
#include "SetKeyContext.h"
#include "TextureArrayRange.h"
#include "shader_builder/ShaderResourceBase.h"
#include "pipeline/FactoryCharacteristicKey.h"
#include "pipeline/FactoryCharacteristicData.h"
#include "vk_utils/TaskToTaskDeque.h"
#include "cwds/debug_ostream_operators.h"
#include <boost/container/flat_map.hpp>

namespace vulkan {
class Texture;

namespace descriptor {
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
class CombinedImageSamplerUpdater : public vk_utils::TaskToTaskDeque<AIStatefulTask, boost::intrusive_ptr<Update>>, public shader_builder::ShaderResourceBase
{
 protected:
  using direct_base_type = AIStatefulTask;

  enum combined_image_sampler_state_type {
    CombinedImageSamplerUpdater_need_action = direct_base_type::state_end,
    CombinedImageSamplerUpdater_done
  };

 public:
  static state_type constexpr state_end = CombinedImageSamplerUpdater_done + 1;

 private:
  task::SynchronousWindow const* m_owning_window{};                             // The owning window.
  std::unique_ptr<detail::CombinedImageSamplerShaderResourceMember> m_member;   // A CombinedImageSamplerUpdater only has a single "member".
  std::atomic<vk::DescriptorBindingFlags> m_binding_flags{};                    // Optional binding flags to use for this descriptor.
  std::atomic<int32_t> m_descriptor_array_size{1};                              // Array size or one if this is not an array, negative when unbounded.
  using factory_characteristic_key_to_descriptor_t = std::vector<std::pair<pipeline::FactoryCharacteristicKey, pipeline::FactoryCharacteristicData>>;
  factory_characteristic_key_to_descriptor_t m_factory_characteristic_key_to_descriptor;        // The descriptor set / bindings associated with this CombinedImageSamplerUpdater.
  using factory_characteristic_key_to_texture_array_range_t = std::vector<std::pair<pipeline::FactoryCharacteristicKey, TextureArrayRange>>;
  factory_characteristic_key_to_texture_array_range_t m_factory_characteristic_key_to_texture_array_range;

 private:
  std::pair<factory_characteristic_key_to_descriptor_t::const_iterator, factory_characteristic_key_to_descriptor_t::const_iterator> find_descriptors(pipeline::FactoryCharacteristicKey const& key) const;
  factory_characteristic_key_to_texture_array_range_t::const_iterator find_texture_array_range(pipeline::FactoryCharacteristicKey const& key) const;

 public:
  CombinedImageSamplerUpdater(char const* glsl_id_full_postfix COMMA_CWDEBUG_ONLY(bool debug = false)) :
    vk_utils::TaskToTaskDeque<AIStatefulTask, boost::intrusive_ptr<Update>>(CWDEBUG_ONLY(debug)), ShaderResourceBase(SetKeyContext::instance(), glsl_id_full_postfix)
  {
    DoutEntering(dc::statefultask(mSMDebug)|dc::vulkan, "descriptor::CombinedImageSamplerUpdater::CombinedImageSamplerUpdater(" << NAMESPACE_DEBUG::print_string(glsl_id_full_postfix) << " [" << this << "]");
    std::string glsl_id_full("CombinedImageSampler::");
    glsl_id_full.append(glsl_id_full_postfix);
    m_member = detail::CombinedImageSamplerShaderResourceMember::create(glsl_id_full);
  }

  // Probably not used.
  CombinedImageSamplerUpdater(CombinedImageSamplerUpdater&&) = default;

  void set_bindings_flags(vk::DescriptorBindingFlags binding_flags)
  {
    DoutEntering(dc::vulkan, "CombinedImageSamplerUpdater::set_bindings_flags(" << binding_flags << ") [" << this << "]");
    // Don't call set_bindings_flags unless you have something to set :p.
    ASSERT(!!binding_flags);
    [[maybe_unused]] vk::DescriptorBindingFlags prev_binding_flags = m_binding_flags.exchange(binding_flags, std::memory_order::relaxed);
    // Don't call set_bindings_flags twice (with different values).
    ASSERT(!prev_binding_flags || prev_binding_flags == binding_flags);
  }

  void set_array_size(uint32_t descriptor_array_size, bool unbounded)
  {
    DoutEntering(dc::vulkan, "CombinedImageSamplerUpdater::set_array_size(" << descriptor_array_size << ", " << (unbounded ? "un" : "") << "bounded) [" << this << "]");
    // Don't call set_array_size unless it's an array :p.
    ASSERT(descriptor_array_size > 1);
    // A negative value means unbounded.
    int32_t size = unbounded ? -descriptor_array_size : descriptor_array_size;
    [[maybe_unused]] int32_t prev_descriptor_array_size = m_descriptor_array_size.exchange(size, std::memory_order::relaxed);
    // Don't call set_array_size twice (with different values).
    ASSERT(prev_descriptor_array_size == 1 || prev_descriptor_array_size == size);
  }

  CombinedImageSamplerUpdater& operator=(CombinedImageSamplerUpdater&& rhs)
  {
    this->ShaderResourceBase::operator=(std::move(rhs));
    m_member = std::move(rhs.m_member);
    m_descriptor_array_size.store(rhs.m_descriptor_array_size.load(std::memory_order::relaxed), std::memory_order::relaxed);
    return *this;
  }

  char const* glsl_id_full() const { return m_member->member().glsl_id_full(); }
  ShaderResourceMember const& member() const { return m_member->member(); }

  // There is no need to instantiate anything for CombinedImageSamplerUpdaters.
  void instantiate(task::SynchronousWindow const* owning_window COMMA_CWDEBUG_ONLY(Ambifix const& ambifix)) override { }

  void prepare_shader_resource_declaration(SetIndexHint set_index_hint, pipeline::AddShaderStage* add_shader_stage) const override final;

  void update_descriptor_set(DescriptorUpdateInfo descriptor_update_info) override final
  {
    DoutEntering(dc::shaderresource, "CombinedImageSamplerUpdater::update_descriptor_set(" << descriptor_update_info << ")");

    boost::intrusive_ptr<Update> update = new DescriptorUpdateInfo(std::move(descriptor_update_info));

    // Pass new descriptors that need to be updated to this task (this is called from a PipelineFactory).
    have_new_datum(std::move(update));
  }

  vk::DescriptorBindingFlags binding_flags() const override final { return m_binding_flags; }
  int32_t descriptor_array_size() const override final { return m_descriptor_array_size; }

 protected:
  ~CombinedImageSamplerUpdater() override;
  char const* task_name_impl() const override { return "CombinedImageSamplerUpdater"; }
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

} // namespace descriptor
} // namespace vulkan
