#pragma once

#include "utils/has_print_on.h"

namespace vulkan::shader_builder {
class ShaderResourceBase;
} // namespace vulkan::shader_builder

namespace vulkan::pipeline {
using utils::has_print_on::operator<<;

class CharacteristicRange;

class ShaderResourcePlusCharacteristic
{
 private:
  shader_builder::ShaderResourceBase const* m_shader_resource;  // Added shader resource.
  pipeline::CharacteristicRange* m_characteristic_range;        // The CharacteristicRange from which it was added.
  int m_fill_index;                                             // The CharacteristicRange value from which it was added.

 public:
  ShaderResourcePlusCharacteristic(shader_builder::ShaderResourceBase const* shader_resource, pipeline::CharacteristicRange* characteristic_range, int fill_index) : m_shader_resource(shader_resource), m_characteristic_range(characteristic_range), m_fill_index(fill_index) { }

  // Accessors.
  shader_builder::ShaderResourceBase const* shader_resource() const { return m_shader_resource; }
  pipeline::CharacteristicRange* characteristic_range() const { return m_characteristic_range; }
  int fill_index() const { return m_fill_index; }
  bool has_unbounded_descriptor_array_size() const;

#ifdef CWDEBUG
 public:
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::pipeline
