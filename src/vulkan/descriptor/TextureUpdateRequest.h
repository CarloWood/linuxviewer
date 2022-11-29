#pragma once

#include "Update.h"
#include "shader_builder/shader_resource/Texture.h"
#include "pipeline/FactoryCharacteristicId.h"
#include "pipeline/FactoryCharacteristicKey.h"
#include "pipeline/ConsecutiveRange.h"

namespace vulkan::descriptor {

class TextureUpdateRequest : public Update
{
 private:
   shader_builder::shader_resource::Texture const* m_texture;
   pipeline::FactoryCharacteristicId const& m_factory_characteristic_id;
   pipeline::ConsecutiveRange m_subrange;
   ArrayElementRange m_array_element_range;                                     // The descriptor array elements to target.

 public:
  TextureUpdateRequest(shader_builder::shader_resource::Texture const* texture, pipeline::FactoryCharacteristicId const& factory_characteristic_id, pipeline::ConsecutiveRange subrange, ArrayElementRange array_element_range) :
    m_texture(texture), m_factory_characteristic_id(factory_characteristic_id), m_subrange(subrange), m_array_element_range(array_element_range) { }

  // Accessors.
  shader_builder::shader_resource::Texture const* texture() const { return m_texture; }
  pipeline::FactoryCharacteristicId const& factory_characteristic_id() const { return m_factory_characteristic_id; }
  pipeline::ConsecutiveRange subrange() const { return m_subrange; }
  ArrayElementRange array_element_range() const { return m_array_element_range; }
  pipeline::FactoryCharacteristicKey key() const { return {m_factory_characteristic_id, m_subrange}; }

  bool is_descriptor_update_info() const override final
  {
    return false;
  }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const override;
#endif
};

} // namespace vulkan::descriptor
