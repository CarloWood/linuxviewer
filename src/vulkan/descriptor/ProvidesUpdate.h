#pragma once

#include "Update.h"
#include "shader_builder/shader_resource/Texture.h"
#include "pipeline/FactoryRangeId.h"

namespace vulkan::descriptor {

class ProvidesUpdate : public Update
{
 private:
   shader_builder::shader_resource::Texture const& m_texture;
   pipeline::FactoryRangeId const& m_factory_range_id;

 public:
  ProvidesUpdate(shader_builder::shader_resource::Texture const& texture, pipeline::FactoryRangeId const& factory_range_id) :
    m_texture(texture), m_factory_range_id(factory_range_id) { }

  bool is_needs_update() const override final
  {
    return false;
  }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const override;
#endif
};

} // namespace vulkan::descriptor
