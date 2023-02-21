#pragma once

#include "vk_utils/print_flags.h"
#include <cstdint>
#include <typeindex>
#include <atomic>
#include "debug.h"

#ifdef CWDEBUG
namespace xml {
class Bridge;
} // namespace xml
#endif

namespace vulkan {

class PushConstantRange
{
 private:
  std::type_index m_type_index;                                 // Type index of the underlaying push constant type (e.g. MyPushConstant).
  uint32_t m_offset;                                            // The offset of this range within that type.
  uint32_t m_size;                                              // The size of this range within that type.
  // Mutable because this object is used with threadsafe-const-ness.
  mutable std::atomic<vk::ShaderStageFlags::MaskType> m_shader_stage_flags;     // The shader stages that use this range.

 public:
  // Construct a range; this is typically used by the user to construct a PushConstantRange object.
  // The Vulkan engine will fill in the m_shader_stage_flags bits.
  //
  // Usage:
  //
  // One could, for example, define the following in the users Window class
  //
  //   vulkan::PushConstantRange m_my_push_constant_range_some_element{
  //     typeid(MyPushConstant), offsetof(MyPushConstant, m_some_element), sizeof(MyPushConstant::m_some_element)};
  //
  // and then pass that to
  //
  //   add_push_constant<MyPushConstant>(window->m_my_push_constant_range_some_element);
  //
  // in the initialize state of an AddPushConstant derived pipeline characteristic.
  //
  PushConstantRange(std::type_index type_index, uint32_t offset, uint32_t size) :
    m_type_index(type_index), m_offset(offset), m_size(size) { }

  // Called from PushConstantDeclarationContext::glsl_id_full_is_used_in when the use of
  // a particular pushconstant range is detected in a given shader stage.
  PushConstantRange(std::type_index type_index, uint32_t offset, uint32_t size, vk::ShaderStageFlagBits shader_stage) :
    m_type_index(type_index), m_offset(offset), m_size(size), m_shader_stage_flags(static_cast<vk::ShaderStageFlags::MaskType>(shader_stage)) { }

  // Also called from PushConstantDeclarationContext::glsl_id_full_is_used_in when the use of
  // a particular pushconstant range is detected in a given shader stage, in order to update
  // the PushConstantRange objects that were constructed with the first constructor.
  // Note: we expect only a single bit to be set in shader_stage, namely the shader stage that this is currently being used in.
  void set_shader_bit(vk::ShaderStageFlags shader_stage) /*threadsafe-*/const
  {
    DoutEntering(dc::vulkan, "PushConstantRange::set_shader_bit(" << shader_stage << ") [" << this << "]");
    CWDEBUG_ONLY(auto prev_flags =)
      m_shader_stage_flags.fetch_or(static_cast<vk::ShaderStageFlags::MaskType>(shader_stage), std::memory_order::relaxed);
#ifdef CWDEBUG
    auto new_flags = prev_flags | static_cast<vk::ShaderStageFlags::MaskType>(shader_stage);
    if (prev_flags != new_flags)
      Dout(dc::vulkan, "Changed " << vk::ShaderStageFlags{prev_flags} << " --> " << vk::ShaderStageFlags{new_flags});
#endif
  }

  PushConstantRange(PushConstantRange const& orig) :
    m_type_index(orig.m_type_index),
    m_shader_stage_flags{orig.m_shader_stage_flags.load(std::memory_order::relaxed)},
    m_offset(orig.m_offset),
    m_size(orig.m_size)
  {
  }

  PushConstantRange& operator=(PushConstantRange const& orig)
  {
    m_type_index = orig.m_type_index;
    m_shader_stage_flags = orig.m_shader_stage_flags.load(std::memory_order::relaxed);
    m_offset = orig.m_offset;
    m_size = orig.m_size;
    return *this;
  }

  // Accessors.
  std::type_index type_index() const { return m_type_index; }
  vk::ShaderStageFlags shader_stage_flags() const { return vk::ShaderStageFlags{m_shader_stage_flags}; }
  uint32_t offset() const { return m_offset; }
  uint32_t size() const { return m_size; }

  friend bool operator<(PushConstantRange const& lhs, PushConstantRange const& rhs);

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;

  friend bool operator==(PushConstantRange const& lhs, PushConstantRange const& rhs);

  // Required for xml serialization.
  PushConstantRange() : m_type_index(typeid(int)) { }
  void xml(xml::Bridge& xml);
#endif
};

} // namespace vulkan
