#pragma once

#include "Update.h"
#include "../TextureArrayRange.h"
#include "../pipeline/FactoryCharacteristicId.h"
#include "../pipeline/FactoryCharacteristicKey.h"
#include "../vk_utils/ConsecutiveRange.h"

namespace vulkan::descriptor {

class TextureUpdateRequest : public Update
{
 private:
   TextureArrayRange m_texture_array_range;                               // The texture(s) to update with.
   pipeline::FactoryCharacteristicId const& m_factory_characteristic_id;  // Only target descriptor sets / bindings associated with this pipeline factory / characteristic range
   vk_utils::ConsecutiveRange m_subrange;                                 // and characteristic range - subrange.

 public:
  TextureUpdateRequest(Texture const* texture, pipeline::FactoryCharacteristicId const& factory_characteristic_id, vk_utils::ConsecutiveRange subrange, ArrayElementRange array_element_range) :
    m_texture_array_range(texture, array_element_range), m_factory_characteristic_id(factory_characteristic_id), m_subrange(subrange) { }

  // Accessors.
  TextureArrayRange const& texture_array_range() const { return m_texture_array_range; }
  pipeline::FactoryCharacteristicId const& factory_characteristic_id() const { return m_factory_characteristic_id; }
  vk_utils::ConsecutiveRange subrange() const { return m_subrange; }
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
