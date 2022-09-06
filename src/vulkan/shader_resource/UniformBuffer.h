#pragma once

#include "FrameResourceIndex.h"
#include "descriptor/SetKey.h"
#include "descriptor/SetKeyContext.h"
#include "shaderbuilder/ShaderVariableLayouts.h"
#include "memory/UniformBuffer.h"
#include "utils/Vector.h"
#ifdef CWDEBUG
#include "debug/vulkan_print_on.h"
#include <cxxabi.h>
#endif

namespace task {
class SynchronousWindow;
} // namespace vulkan

namespace vulkan::shader_resource {

class UniformBufferBase
{
 protected:
  descriptor::SetKey const m_descriptor_set_key;
  std::string m_glsl_id_str_prefix;
  utils::Vector<memory::UniformBuffer, FrameResourceIndex> m_uniform_buffers;

 public:
  UniformBufferBase(std::string_view glsl_id_str_prefix) : m_descriptor_set_key(descriptor::SetKeyContext::instance()), m_glsl_id_str_prefix(glsl_id_str_prefix) { }

  void create(task::SynchronousWindow const* owning_window, vk::DeviceSize size
      COMMA_CWDEBUG_ONLY(Ambifix const& ambifix));

  // Accessors.
  descriptor::SetKey descriptor_set_key() const { return m_descriptor_set_key; }
  std::string const& glsl_id_str_prefix() const { return m_glsl_id_str_prefix; }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

template<typename ENTRY>
struct UniformBufferInstance : public memory::UniformBuffer
{
  // This class may not define any members!
};

// Represents the descriptor set of a uniform buffer.
template<typename ENTRY>
class UniformBuffer : public UniformBufferBase
{
 public:
  // Use create to initialize m_uniform_buffers.
  UniformBuffer();

  UniformBufferInstance<ENTRY>& operator[](FrameResourceIndex frame_resource_index) { return static_cast<UniformBufferInstance<ENTRY>&>(m_uniform_buffers[frame_resource_index]); }
  UniformBufferInstance<ENTRY> const& operator[](FrameResourceIndex frame_resource_index) const { return static_cast<UniformBufferInstance<ENTRY> const&>(m_uniform_buffers[frame_resource_index]); }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

// Helper class to extract the glsl_id_str prefix from an ENTRY type.
template<typename ENTRY, int member>
struct GlslIdStr
{
  static constexpr utils::TemplateStringLiteral glsl_id_str =
    std::tuple_element_t<member, typename decltype(vulkan::shaderbuilder::ShaderVariableLayouts<ENTRY>::struct_layout)::members_tuple>::glsl_id_str;
  static constexpr std::string_view glsl_id_sv = static_cast<std::string_view>(glsl_id_str);
  static constexpr std::string_view prefix{glsl_id_sv.data(), glsl_id_sv.find(':')};
};

template<typename ENTRY>
UniformBuffer<ENTRY>::UniformBuffer() : UniformBufferBase(GlslIdStr<ENTRY, 0>::prefix)
{
}

template<typename ENTRY>
void UniformBuffer<ENTRY>::print_on(std::ostream& os) const
{
  os << "(shader_resource/UniformBuffer<" << libcwd::type_info_of<ENTRY>().demangled_name() << ">)";
  UniformBufferBase::print_on(os);
}

} // namespace vulkan::shader_resource
