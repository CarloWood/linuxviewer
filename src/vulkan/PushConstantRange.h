#pragma once

#include <vulkan/vulkan.hpp>
#include <cstdint>
#include <typeindex>
#ifdef CWDEBUG
#include <iosfwd>
#endif

namespace vulkan {

class PushConstantRange
{
 private:
  std::type_index m_type_index;                 // Type index of the underlaying push constant type (e.g. MyPushConstant).
  vk::ShaderStageFlags m_shader_stage_flags;    // The shader stages that use this range.
  uint32_t m_offset;                            // The offset of this range within that type.
  uint32_t m_size;                              // The size of this range within that type.

 public:
  PushConstantRange(std::type_index type_index, vk::ShaderStageFlags shader_stage_flags, uint32_t offset, uint32_t size) :
    m_type_index(type_index), m_shader_stage_flags(shader_stage_flags), m_offset(offset), m_size(size) { }

  // Accessors.
  std::type_index type_index() const { return m_type_index; }
  vk::ShaderStageFlags shader_stage_flags() const { return m_shader_stage_flags; }
  uint32_t offset() const { return m_offset; }
  uint32_t size() const { return m_size; }

  friend bool operator<(PushConstantRange const& lhs, PushConstantRange const& rhs);

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan
