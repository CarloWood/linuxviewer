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
  descriptor::SetKey const m_descriptor_set_key;                                // A unique key for this shader resource.
  std::vector<char const*> m_glsl_id_strs;                                      // The glsl_id_str's of each of the members of ENTRY (of the derived class).
  utils::Vector<memory::UniformBuffer, FrameResourceIndex> m_uniform_buffers;   // The actual uniform buffer(s), one for each frame resource.

 public:
  UniformBufferBase(int number_of_members) : m_descriptor_set_key(descriptor::SetKeyContext::instance()), m_glsl_id_strs(number_of_members) { }

  // Create the memory::UniformBuffer's of m_uniform_buffers.
  void create(task::SynchronousWindow const* owning_window, vk::DeviceSize size
      COMMA_CWDEBUG_ONLY(Ambifix const& ambifix));

  template<typename ContainingClass, glsl::Standard Standard, glsl::ScalarIndex ScalarIndex, int Rows, int Cols, size_t Alignment, size_t Size, size_t ArrayStride,
      int MemberIndex, size_t MaxAlignment, size_t Offset, utils::TemplateStringLiteral GlslIdStr>
  void add_uniform_buffer_member(shaderbuilder::MemberLayout<ContainingClass,
      shaderbuilder::BasicTypeLayout<Standard, ScalarIndex, Rows, Cols, Alignment, Size, ArrayStride>,
      MemberIndex, MaxAlignment, Offset, GlslIdStr> const& member_layout);

  // Accessors.
  descriptor::SetKey descriptor_set_key() const { return m_descriptor_set_key; }
  std::vector<char const*> const& glsl_id_strs() const { return m_glsl_id_strs; }

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

#if 0
// Helper class to extract the glsl_id_str prefix from an ENTRY type.
template<typename ENTRY, int member>
struct GlslIdStr
{
  static constexpr utils::TemplateStringLiteral glsl_id_str =
    std::tuple_element_t<member, typename decltype(vulkan::shaderbuilder::ShaderVariableLayouts<ENTRY>::struct_layout)::members_tuple>::glsl_id_str;
  static constexpr std::string_view glsl_id_sv = static_cast<std::string_view>(glsl_id_str);
  static constexpr std::string_view prefix{glsl_id_sv.data(), glsl_id_sv.find(':')};
};
#endif

template<typename ContainingClass, glsl::Standard Standard, glsl::ScalarIndex ScalarIndex, int Rows, int Cols, size_t Alignment, size_t Size, size_t ArrayStride,
    int MemberIndex, size_t MaxAlignment, size_t Offset, utils::TemplateStringLiteral GlslIdStr>
void UniformBufferBase::add_uniform_buffer_member(shaderbuilder::MemberLayout<ContainingClass,
    shaderbuilder::BasicTypeLayout<Standard, ScalarIndex, Rows, Cols, Alignment, Size, ArrayStride>,
    MemberIndex, MaxAlignment, Offset, GlslIdStr> const& member_layout)
{
  // Paranoia: this vector should already have been resized.
  ASSERT(MemberIndex < m_glsl_id_strs.size());
  std::string_view glsl_id_sv = static_cast<std::string_view>(member_layout.glsl_id_str);

  m_glsl_id_strs[MemberIndex] = static_cast<std::string_view>(GlslIdStr).data();
}

template<typename ENTRY>
UniformBuffer<ENTRY>::UniformBuffer() : UniformBufferBase(shaderbuilder::ShaderVariableLayouts<ENTRY>::struct_layout.members)
{
  using namespace shaderbuilder;
  // Use the specialization of ShaderVariableLayouts to get the layout of ENTRY
  // in the form of a tuple, of the uniform buffer struct members `layouts`.
  // Then for each member layout call add_uniform_buffer_member.
  [&]<typename... MemberLayout>(std::tuple<MemberLayout...> const& layouts)
  {
    ([&, this]
    {
      {
#if 0 // Print the type MemberLayout.
        int rc;
        char const* const demangled_type = abi::__cxa_demangle(typeid(MemberLayout).name(), 0, 0, &rc);
        Dout(dc::vulkan, "We get here for type " << demangled_type);
#endif
        static constexpr int member_index = MemberLayout::member_index;
        add_uniform_buffer_member(std::get<member_index>(layouts));
      }
    }(), ...);
  }(typename decltype(ShaderVariableLayouts<ENTRY>::struct_layout)::members_tuple{});
}

template<typename ENTRY>
void UniformBuffer<ENTRY>::print_on(std::ostream& os) const
{
  os << "(shader_resource/UniformBuffer<" << libcwd::type_info_of<ENTRY>().demangled_name() << ">)";
  UniformBufferBase::print_on(os);
}

} // namespace vulkan::shader_resource
