#ifndef SHADER_RESOURCE_COMBINED_IMAGE_SAMPLER_H
#define SHADER_RESOURCE_COMBINED_IMAGE_SAMPLER_H

#include "descriptor/CombinedImageSamplerUpdater.h"
#include "descriptor/TextureUpdateRequest.h"

namespace vulkan::shader_builder::shader_resource {

class CombinedImageSampler
{
 public:
  enum ArrayType
  {
    bounded_array,
    unbounded_array
  };

 private:
  boost::intrusive_ptr<task::CombinedImageSamplerUpdater> m_descriptor_task;

 public:
  // Note: the call to set_glsl_id_postfix creates and runs m_descriptor_task,
  // but that task won't do anything until after the same thread had a chance
  // to call set_array_size (if this is an array) to finish the initialization.
  void set_glsl_id_postfix(char const* glsl_id_full_postfix);
  void set_bindings_flags(vk::DescriptorBindingFlagBits binding_flags);
  void set_array_size(uint32_t array_size, ArrayType array_type = bounded_array);

  void update_image_sampler_array(Texture const* texture, pipeline::FactoryCharacteristicId const& factory_characteristic_id, vk_utils::ConsecutiveRange subrange, descriptor::ArrayElementRange array_element_range)
  {
    DoutEntering(dc::shaderresource, "CombinedImageSampler::update_image_sampler(" << texture << ", " << factory_characteristic_id << ", " << subrange << ", " << array_element_range << ")");

    boost::intrusive_ptr<descriptor::Update> update = new descriptor::TextureUpdateRequest(texture, factory_characteristic_id, subrange, array_element_range);

    // Pass new image samplers that we need to update the descriptors with.
    // Picked up by CombinedImageSamplerUpdater_need_action in CombinedImageSamplerUpdater::multiplex_impl.
    m_descriptor_task->have_new_datum(std::move(update));
  }

  void update_image_sampler_array(Texture const* texture, pipeline::FactoryCharacteristicId const& factory_characteristic_id, descriptor::ArrayElementRange array_element_range)
  {
    update_image_sampler_array(texture, factory_characteristic_id, factory_characteristic_id.full_range(), array_element_range);
  }

  void update_image_sampler(Texture const* texture, pipeline::FactoryCharacteristicId const& factory_characteristic_id, vk_utils::ConsecutiveRange subrange)
  {
    update_image_sampler_array(texture, factory_characteristic_id, subrange, 0);
  }

  void update_image_sampler(Texture const* texture, pipeline::FactoryCharacteristicId const& factory_characteristic_id)
  {
    update_image_sampler_array(texture, factory_characteristic_id, factory_characteristic_id.full_range(), 0);
  }

  // Accessor.
  task::CombinedImageSamplerUpdater const* descriptor_task() const { return m_descriptor_task.get(); }

#ifdef CWDEBUG
 public:
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::shader_builder::shader_resource

#endif // SHADER_RESOURCE_COMBINED_IMAGE_SAMPLER_H
