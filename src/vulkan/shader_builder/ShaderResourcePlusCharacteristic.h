#pragma once

#include "utils/has_print_on.h"

namespace vulkan::pipeline {
class CharacteristicRange;
} // namespace pipeline

namespace vulkan::shader_builder {
using utils::has_print_on::operator<<;

namespace shader_resource {
class Base;
} // namespace shader_resource

class ShaderResourcePlusCharacteristic
{
 private:
  shader_resource::Base const* m_shader_resource;               // Added shader resource.
  pipeline::CharacteristicRange const* m_characteristic_range;  // The CharacteristicRange from which it was added.
  int m_fill_index;                                             // The CharacteristicRange value from which it was added.

 public:
  ShaderResourcePlusCharacteristic(shader_resource::Base const* shader_resource, pipeline::CharacteristicRange const* characteristic_range, int fill_index) : m_shader_resource(shader_resource), m_characteristic_range(characteristic_range), m_fill_index(fill_index) { }

  // Accessors.
  shader_resource::Base const* shader_resource() const { return m_shader_resource; }
  pipeline::CharacteristicRange const* characteristic_range() const { return m_characteristic_range; }
  int fill_index() const { return m_fill_index; }

#ifdef CWDEBUG
 public:
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::shader_builder
