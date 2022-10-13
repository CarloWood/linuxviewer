#pragma once

#include "math/glsl.h"
#include "utils/TemplateStringLiteral.h"
#include <vulkan/vulkan.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <functional>
#include <cstring>
#include <iosfwd>
#include <map>
#include <cstdint>
#include <string>
#include <type_traits>
#include <numeric>
#include "debug.h"

namespace vulkan_shader_builder_specialization_classes {

// Base class of ShaderVariableLayouts in order to declare containing_class and containing_class_name.
// See LAYOUT_DECLARATION.
template<typename ENTRY>
struct ShaderVariableLayoutsBase;

// This template struct must be specialized for each shader variable struct.
// See VertexShaderInputSet for more info and example.
template<typename ENTRY>
struct ShaderVariableLayouts;

} // namespace vulkan_shader_builder_specialization_classes

namespace vulkan::shader_builder {
using namespace vulkan_shader_builder_specialization_classes;

// This template struct is specialized below for BasicTypeLayout, MemberLayout, ArrayLayout and StructLayout.
// It provides a generic interface to the common parameters of a layout: alignment, size and array_stride.
template<typename XLayout>
struct Layout
{
  // This is unused, unless the ConceptXLayout fails.
};

// BasicTypeLayout
//
//     Standard : the standard that this layout conforms to.
//  ScalarIndex : the underlying scalar type (float, double, bool, int or uint), as ScalarIndex enum (eFloat, eDouble, eBool, eInt or eUint).
//         Rows : the number of rows of vector or matrix, or 1 if this is a scalar (in that case cols will also be 1).
//         Cols : the number of columns of this is a matrix, or 1 if this is a scalar or vector.
//    Alignment : the alignment of this base type according to the specified standard.
//  ArrayStride : the array stride of this type according to the specified standard.
//
template<glsl::Standard Standard, glsl::ScalarIndex ScalarIndex, int Rows, int Cols, size_t Alignment, size_t Size, size_t ArrayStride>
struct BasicTypeLayout
{
};

template<glsl::Standard Standard, glsl::ScalarIndex ScalarIndex, int Rows, int Cols, size_t Alignment, size_t Size, size_t ArrayStride>
struct Layout<BasicTypeLayout<Standard, ScalarIndex, Rows, Cols, Alignment, Size, ArrayStride>>
{
  using xlayout_t = BasicTypeLayout<Standard, ScalarIndex, Rows, Cols, Alignment, Size, ArrayStride>;
  static constexpr glsl::Standard standard = Standard;
  static constexpr size_t alignment = Alignment;
  static constexpr size_t size = Size;
  static constexpr size_t array_stride = ArrayStride;
};

// MemberLayout
//
// ContainingClass : the class or struct that contains this member.
//         XLayout : the layout of the member (BasicTypeLayout, ArrayLayout or StructLayout).
//     MemberIndex : the position of this member in the struct (the first member of the struct has index 0).
//    MaxAlignment : the largest value of all alignments of this and all previous members of the struct that this is a member of.
//          Offset : the offset in bytes of this member relative to the beginning of the struct that this is a member of.
//
template<typename ContainingClass, typename XLayout, int MemberIndex, size_t MaxAlignment, size_t Offset, utils::TemplateStringLiteral GlslIdStr>
struct MemberLayout
{
  using containing_class = ContainingClass;
  using layout_type = XLayout;
  static constexpr glsl::Standard standard = XLayout::standard;
  static constexpr int member_index = MemberIndex;
  static constexpr size_t max_alignment = MaxAlignment;
  static constexpr size_t offset = Offset;
  static constexpr utils::TemplateStringLiteral glsl_id_full = GlslIdStr;
};

template<typename ContainingClass, typename XLayout, int Index, size_t MaxAlignment, size_t Offset, utils::TemplateStringLiteral GlslIdStr>
struct Layout<MemberLayout<ContainingClass, XLayout, Index, MaxAlignment, Offset, GlslIdStr>>
{
  using xlayout_t = MemberLayout<ContainingClass, XLayout, Index, MaxAlignment, Offset, GlslIdStr>;
  static constexpr size_t alignment = Layout<XLayout>::alignment;
  static constexpr size_t size = Layout<XLayout>::size;
  static constexpr size_t array_stride = Layout<XLayout>::array_stride;
};

// ArrayLayout
//
//  XLayout : the layout of one element of the array (when it would not be part of an array).
// Elements : the size of the array.
//
template<typename XLayout, size_t Elements>
struct ArrayLayout
{
  using element_xlayout_t = XLayout;
  static constexpr glsl::Standard standard = XLayout::standard;
};

template<typename XLayout, size_t Elements>
struct Layout<ArrayLayout<XLayout, Elements>>
{
  using xlayout_t = ArrayLayout<XLayout, Elements>;
  static constexpr size_t alignment = Layout<XLayout>::alignment;
  static constexpr size_t size = Layout<XLayout>::array_stride * Elements;
  static constexpr size_t array_stride = size;
};

// StructLayout
//
// MembersTuple : A tuple of all MemberLayout's.
//
template<typename MembersTuple>
struct StructLayout
{
  using members_tuple = MembersTuple;
  static constexpr int members = std::tuple_size_v<MembersTuple>;
  static_assert(members > 0, "There must be at least one member in a StructLayout");
  using last_member_layout = std::tuple_element_t<members - 1, MembersTuple>;
  using underlying_class = typename last_member_layout::containing_class;        // Should be the same for every member.
  static constexpr glsl::Standard standard = last_member_layout::standard;
  static constexpr size_t alignment = last_member_layout::max_alignment;
  static constexpr size_t size = round_up_to_multiple_off(last_member_layout::offset + Layout<typename last_member_layout::layout_type>::size, alignment);
  static constexpr std::string_view last_member_glsl_id_sv = static_cast<std::string_view>(last_member_layout::glsl_id_full);
  static constexpr size_t underlying_class_name_len = last_member_glsl_id_sv.find(':');
  static constexpr utils::TemplateStringLiteral<underlying_class_name_len> glsl_id_full{last_member_glsl_id_sv.data(), underlying_class_name_len};

  constexpr StructLayout(MembersTuple) {}
};

template<typename MembersTuple>
struct Layout<StructLayout<MembersTuple>>
{
  static constexpr size_t alignment = StructLayout<MembersTuple>::alignment;
  static constexpr size_t size = StructLayout<MembersTuple>::size;
  static constexpr size_t array_stride = std::max({size, alignment, (StructLayout<MembersTuple>::standard == glsl::std140) ? 16 : 0});
};

template<typename XLayout>
concept ConceptXLayout = requires
{
  typename Layout<XLayout>::xlayout_t;
};

template<typename ContainingClass, typename XLayout, utils::TemplateStringLiteral MemberName>
struct StructMember;

// The type returned by the macro LAYOUT.
template<typename ContainingClass, ConceptXLayout XLayout, utils::TemplateStringLiteral MemberName>
struct StructMember<ContainingClass, XLayout, MemberName>
{
  static constexpr size_t alignment = Layout<XLayout>::alignment;
  static constexpr utils::TemplateStringLiteral glsl_id_full = utils::Catenate_v<vulkan_shader_builder_specialization_classes::ShaderVariableLayoutsBase<ContainingClass>::prefix, MemberName>;
  using containing_class = ContainingClass;
  using layout_type = XLayout;
};

// Specialization for arrays.
template<typename ContainingClass, ConceptXLayout XLayout, utils::TemplateStringLiteral MemberName, size_t Elements>
struct StructMember<ContainingClass, XLayout[Elements], MemberName>
{
  static constexpr utils::TemplateStringLiteral glsl_id_full = utils::Catenate_v<vulkan_shader_builder_specialization_classes::ShaderVariableLayoutsBase<ContainingClass>::prefix, MemberName>;
  static constexpr size_t alignment = Layout<XLayout>::alignment;
  using containing_class = ContainingClass;
  using layout_type = ArrayLayout<XLayout, Elements>;
};

#define LAYOUT_DECLARATION(classname, standard) \
  template<> \
  struct vulkan_shader_builder_specialization_classes::ShaderVariableLayoutsBase<classname> \
  { \
    using containing_class = classname; \
    using glsl_standard = glsl::standard; \
    static constexpr std::array long_prefix_a = std::to_array(BOOST_PP_STRINGIZE(classname)"::"); \
    static constexpr std::string_view long_prefix{long_prefix_a.data(), long_prefix_a.size()}; \
    static constexpr std::string_view prefix_sv = long_prefix.substr(long_prefix.find_last_of(':', long_prefix.size() - 4) + 1); \
    static constexpr utils::TemplateStringLiteral<prefix_sv.size()> prefix{prefix_sv.data(), prefix_sv.size()}; \
  }; \
  \
  template<> \
  struct vulkan_shader_builder_specialization_classes::ShaderVariableLayouts<classname> : glsl::standard, vulkan_shader_builder_specialization_classes::ShaderVariableLayoutsBase<classname>

#define LAYOUT(membertype, membername) \
  (::vulkan::shader_builder::StructMember<containing_class, membertype, BOOST_PP_STRINGIZE(membername)>{})

#define STRUCT_DECLARATION(classname) \
  struct classname : glsl::Data<classname>

#define MEMBER(member_index, membertype, membername) \
  glsl::AlignAndResize<membertype, layout<member_index>> membername

//=============================================================================
// Helper classes for constructing StructLayout<> types.
//

// Run up `val` to the next multiple of `factor`.
consteval size_t round_up_to_multiple_off(size_t val, size_t factor)
{
  return (val + factor - 1) - (val + factor - 1) % factor;
}

template<typename PreviousMemberLayout, typename StructMember>
consteval size_t compute_max_alignment()
{
  return std::max(PreviousMemberLayout::max_alignment, StructMember::alignment);
}

template<typename PreviousMemberLayout, typename StructMember>
consteval size_t compute_offset()
{
  return round_up_to_multiple_off(PreviousMemberLayout::offset + Layout<PreviousMemberLayout>::size, StructMember::alignment);
}

template<typename... MemberLayouts>
constexpr auto make_layouts_impl(std::tuple<MemberLayouts...> layouts)
{
  return layouts;
}

template<typename... MemberLayouts, typename FirstUnprocessed, typename... RestUnprocessed>
constexpr auto make_layouts_impl(std::tuple<MemberLayouts...> layouts, FirstUnprocessed const& first_unprocessed, RestUnprocessed const&... unprocessedMembers)
{
  using LastMemberLayout = std::tuple_element_t<sizeof...(MemberLayouts) - 1, std::tuple<MemberLayouts...>>;

  using NextLayout = MemberLayout<
      typename FirstUnprocessed::containing_class,
      typename FirstUnprocessed::layout_type,
      sizeof...(MemberLayouts),
      compute_max_alignment<LastMemberLayout, FirstUnprocessed>(),
      compute_offset<LastMemberLayout, FirstUnprocessed>(),
      FirstUnprocessed::glsl_id_full
    >;

  return make_layouts_impl(std::tuple_cat(layouts, std::tuple<NextLayout>{}), unprocessedMembers...);
}

template<typename FirstMember, typename... RestMembers>
constexpr auto make_struct_layout(FirstMember const& first_member, RestMembers const&... restMembers)
{
  using ContainingClass = typename FirstMember::containing_class;
  using FirstType = typename FirstMember::layout_type;
  using FirstMemberLayout = MemberLayout<ContainingClass, FirstType, 0/*index*/, Layout<FirstType>::alignment, 0/*offset*/, FirstMember::glsl_id_full>;

  return StructLayout{make_layouts_impl(std::tuple(FirstMemberLayout{}), restMembers...)};
}

// End of Helper classes for constructing StructLayout<> types.
//=============================================================================

namespace standards {
using Standard = glsl::Standard;
using ScalarIndex = glsl::ScalarIndex;

using ScalarIndex::eFloat;
using ScalarIndex::eDouble;
using ScalarIndex::eBool;
using ScalarIndex::eInt;
using ScalarIndex::eUint;
using ScalarIndex::eInt8;
using ScalarIndex::eUint8;
using ScalarIndex::eInt16;
using ScalarIndex::eUint16;

#define SHADERBUILDER_STANDARD vertex_attributes
#include "basic_types.h"
#undef SHADERBUILDER_STANDARD

#define SHADERBUILDER_STANDARD std140
#include "basic_types.h"
#undef SHADERBUILDER_STANDARD

#define SHADERBUILDER_STANDARD std430
#include "basic_types.h"
#undef SHADERBUILDER_STANDARD

#define SHADERBUILDER_STANDARD scalar
#include "basic_types.h"
#undef SHADERBUILDER_STANDARD

} // namespace standards

} // namespace vulkan::shader_builder

// Not sure where to put the 'standards' structs... lets put them in glsl for now.
namespace glsl {

// From https://registry.khronos.org/OpenGL/specs/gl/GLSLangSpec.4.60.html 4.4.5. Uniform and Shader Storage Block Layout Qualifiers:
//
// The std140 and std430 qualifiers override only the packed, shared, std140, and std430 qualifiers; other qualifiers are inherited.
// The std430 qualifier is supported only for shader storage blocks; a shader using the std430 qualifier on a uniform block will
// result in a compile-time error, unless it is also declared with push_constant.
//
// There is an extention that allows to use std430, but we don't use that yet.
struct uniform_std140 : vulkan::shader_builder::standards::std140::TypeEncodings
{
  using tag_type = uniform_std140;
};

// From the same paragraph:
//
// However, when push_constant is declared, the default layout of the buffer will be std430. There is no method to globally set this default.
struct push_constant_std430 : vulkan::shader_builder::standards::std430::TypeEncodings
{
  using tag_type = push_constant_std430;
};

// The following layouts might require an extension.
#if 1
struct uniform_std430 : vulkan::shader_builder::standards::std430::TypeEncodings
{
  using tag_type = uniform_std430;
};

struct uniform_scalar : vulkan::shader_builder::standards::scalar::TypeEncodings
{
  using tag_type = uniform_scalar;
};

struct push_constant_scalar : vulkan::shader_builder::standards::scalar::TypeEncodings
{
  using tag_type = push_constant_scalar;
};
#endif

struct BasicTypesBase
{
  using Float   = glsl::Float;
  using vec2    = glsl::vec2;
  using vec3    = glsl::vec3;
  using vec4    = glsl::vec4;
  using mat2    = glsl::mat2;
  using mat3x2  = glsl::mat3x2;
  using mat4x2  = glsl::mat4x2;
  using mat2x3  = glsl::mat2x3;
  using mat3    = glsl::mat3;
  using mat4x3  = glsl::mat4x3;
  using mat2x4  = glsl::mat2x4;
  using mat3x4  = glsl::mat3x4;
  using mat4    = glsl::mat4;

  using Double  = glsl::Double;
  using dvec2   = glsl::dvec2;
  using dvec3   = glsl::dvec3;
  using dvec4   = glsl::dvec4;
  using dmat2   = glsl::dmat2;
  using dmat3x2 = glsl::dmat3x2;
  using dmat4x2 = glsl::dmat4x2;
  using dmat2x3 = glsl::dmat2x3;
  using dmat3   = glsl::dmat3;
  using dmat4x3 = glsl::dmat4x3;
  using dmat2x4 = glsl::dmat2x4;
  using dmat3x4 = glsl::dmat3x4;
  using dmat4   = glsl::dmat4;

  using Bool    = glsl::Bool;
  using bvec2   = glsl::bvec2;
  using bvec3   = glsl::bvec3;
  using bvec4   = glsl::bvec4;

  using Int     = glsl::Int;
  using ivec2   = glsl::ivec2;
  using ivec3   = glsl::ivec3;
  using ivec4   = glsl::ivec4;

  using Uint    = glsl::Uint;
  using uvec2   = glsl::uvec2;
  using uvec3   = glsl::uvec3;
  using uvec4   = glsl::uvec4;
};

template<Standard>
struct BasicTypes : BasicTypesBase
{
};

template<>
struct BasicTypes<vertex_attributes> : BasicTypesBase
{
  using Int8    = glsl::Int8;
  using i8vec2  = glsl::i8vec2;
  using i8vec3  = glsl::i8vec3;
  using i8vec4  = glsl::i8vec4;

  using Uint8   = glsl::Uint8;
  using u8vec2  = glsl::u8vec2;
  using u8vec3  = glsl::u8vec3;
  using u8vec4  = glsl::u8vec4;

  using Int16   = glsl::Int16;
  using i16vec2 = glsl::i16vec2;
  using i16vec3 = glsl::i16vec3;
  using i16vec4 = glsl::i16vec4;

  using Uint16  = glsl::Uint16;
  using u16vec2 = glsl::u16vec2;
  using u16vec3 = glsl::u16vec3;
  using u16vec4 = glsl::u16vec4;
};

template<size_t padding, size_t alignment>
struct alignas(alignment) AlignedPadding
{
  char __padding[padding];
};

template<size_t alignment>
struct alignas(alignment) AlignedPadding<0, alignment>
{
};

template<typename T>
struct WrappedType : T
{
};

template<typename T>
concept ConceptBaseType = std::is_same_v<T, float> || std::is_same_v<T, double> || std::is_same_v<T, bool> || std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t>;

// Note: this type is only used for declaration; so conversion to T upon using it in any way is perfectly OK.
template<ConceptBaseType T>
struct Builtin
{
  alignas(std::is_same_v<T, double> ? 8 : 4) T m_val;

  Builtin(T val = {}) : m_val(val) { }
  operator T() const { return m_val; }
  operator T&() { return m_val; }
  Builtin& operator=(T val) { m_val = val; return *this; }
};

template<ConceptBaseType T>
struct WrappedType<T> : Builtin<T>
{
  using Builtin<T>::Builtin;
};

template<typename T, typename Layout>
struct AlignAndResize : WrappedType<T>
{
  static constexpr size_t required_alignment = Layout::alignment;
  // In C++ the size must always be a multiple of the alignment. Therefore, we have to possibly relax
  // the (C++ enforced) alignment or the resulting size will be too large!
  // In order to get the right alignment in that case, padding must be added before WrappedType<T>,
  // which will depend on the previous elements.
  static constexpr size_t enforced_alignment = std::gcd(required_alignment, Layout::size);
  static constexpr size_t rounded_up_size = (sizeof(T) + enforced_alignment - 1) / enforced_alignment * enforced_alignment;
  static_assert(Layout::size >= rounded_up_size, "You can't request a size that is less than the actual size of T.");
  static constexpr size_t required_padding = Layout::size - rounded_up_size;

  using WrappedType<T>::WrappedType;

  [[no_unique_address]] AlignedPadding<required_padding, enforced_alignment> __padding;
};

template<typename Layout>
struct ArrayElement
{
  static constexpr size_t alignment = Layout::alignment;
  static constexpr size_t size = Layout::array_stride;
};

template<typename T, typename Layout, size_t Elements>
struct AlignAndResize<T[Elements], Layout> :
    std::array<AlignAndResize<T, ArrayElement<vulkan::shader_builder::Layout<typename Layout::xlayout_t::element_xlayout_t>>>, Elements>
{
  // Array of arrays? Not supported.
  static_assert(Elements != 0);
};

template<typename ContainingClass>
struct Data : BasicTypes<vulkan_shader_builder_specialization_classes::ShaderVariableLayoutsBase<ContainingClass>::glsl_standard::TypeEncodings::glsl_standard>
{
  using containing_class = ContainingClass;
  template<int member_index>
  using layout = vulkan::shader_builder::Layout<typename std::tuple_element_t<member_index, typename decltype(vulkan::shader_builder::ShaderVariableLayouts<ContainingClass>::struct_layout)::members_tuple>::layout_type>;
};

} // glsl
