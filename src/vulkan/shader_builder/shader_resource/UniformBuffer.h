#pragma once

#include "FrameResourceIndex.h"
#include "descriptor/SetKey.h"
#include "descriptor/SetKeyContext.h"
#include "shader_builder/ShaderVariableLayouts.h"
#include "shader_builder/ShaderResourceMember.h"
#include "shader_builder/ShaderResource.h"
#include "memory/UniformBuffer.h"
#include "utils/Vector.h"
#ifdef CWDEBUG
#include "debug/vulkan_print_on.h"
#include <cxxabi.h>
#endif

namespace task {
class SynchronousWindow;
} // namespace vulkan

namespace vulkan::shader_builder::shader_resource {

class UniformBufferBase
{
 protected:
  descriptor::SetKey const m_descriptor_set_key;                                // A unique key for this shader resource.
//  std::vector<char const*> m_glsl_id_fulls;                                      // The glsl_id_full's of each of the members of ENTRY (of the derived class).
  ShaderResource::members_container_t m_members;                                // The members of ENTRY (of the derived class).
  utils::Vector<memory::UniformBuffer, FrameResourceIndex> m_uniform_buffers;   // The actual uniform buffer(s), one for each frame resource.

 public:
  UniformBufferBase(int number_of_members) : m_descriptor_set_key(descriptor::SetKeyContext::instance()) { }

  // Create the memory::UniformBuffer's of m_uniform_buffers.
  void create(task::SynchronousWindow const* owning_window, vk::DeviceSize size
      COMMA_CWDEBUG_ONLY(Ambifix const& ambifix));

  template<typename ContainingClass, glsl::Standard Standard, glsl::ScalarIndex ScalarIndex, int Rows, int Cols, size_t Alignment, size_t Size, size_t ArrayStride,
      int MemberIndex, size_t MaxAlignment, size_t Offset, utils::TemplateStringLiteral GlslIdStr>
  void add_uniform_buffer_member(shader_builder::MemberLayout<ContainingClass,
      shader_builder::BasicTypeLayout<Standard, ScalarIndex, Rows, Cols, Alignment, Size, ArrayStride>,
      MemberIndex, MaxAlignment, Offset, GlslIdStr> const& member_layout);

  // Accessors.
  descriptor::SetKey descriptor_set_key() const { return m_descriptor_set_key; }
  ShaderResource::members_container_t* members() { return &m_members; }
  std::string glsl_id() const;

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
// Helper class to extract the glsl_id_full prefix from an ENTRY type.
template<typename ENTRY, int member>
struct GlslIdStr
{
  static constexpr utils::TemplateStringLiteral glsl_id_full =
    std::tuple_element_t<member, typename decltype(vulkan::shader_builder::ShaderVariableLayouts<ENTRY>::struct_layout)::members_tuple>::glsl_id_full;
  static constexpr std::string_view glsl_id_sv = static_cast<std::string_view>(glsl_id_full);
  static constexpr std::string_view prefix{glsl_id_sv.data(), glsl_id_sv.find(':')};
};
#endif

template<typename ContainingClass, glsl::Standard Standard, glsl::ScalarIndex ScalarIndex, int Rows, int Cols, size_t Alignment, size_t Size, size_t ArrayStride,
    int MemberIndex, size_t MaxAlignment, size_t Offset, utils::TemplateStringLiteral GlslIdStr>
void UniformBufferBase::add_uniform_buffer_member(shader_builder::MemberLayout<ContainingClass,
    shader_builder::BasicTypeLayout<Standard, ScalarIndex, Rows, Cols, Alignment, Size, ArrayStride>,
    MemberIndex, MaxAlignment, Offset, GlslIdStr> const& member_layout)
{
  std::string_view glsl_id_sv = static_cast<std::string_view>(member_layout.glsl_id_full);

  shader_builder::BasicType const basic_type = { .m_standard = Standard, .m_rows = Rows, .m_cols = Cols, .m_scalar_type = ScalarIndex,
    .m_log2_alignment = utils::log2(Alignment), .m_size = Size, .m_array_stride = ArrayStride };

  m_members.try_emplace(MemberIndex, glsl_id_sv.data(), MemberIndex, basic_type, Offset);
}

template<typename ENTRY>
UniformBuffer<ENTRY>::UniformBuffer() : UniformBufferBase(shader_builder::ShaderVariableLayouts<ENTRY>::struct_layout.members)
{
  using namespace shader_builder;
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

#ifdef CWDEBUG
template<typename ENTRY>
void UniformBuffer<ENTRY>::print_on(std::ostream& os) const
{
  os << "(shader_resource/UniformBuffer<" << libcwd::type_info_of<ENTRY>().demangled_name() << ">)";
  UniformBufferBase::print_on(os);
}
#endif

} // namespace vulkan::shader_builder::shader_resource
