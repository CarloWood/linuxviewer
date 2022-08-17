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
#include "debug.h"

namespace vulkan_shaderbuilder_specialization_classes {

// Base class of ShaderVariableLayouts in order to declare containing_class and containing_class_name.
// See LAYOUT_DECLARATION.
template<typename ENTRY>
struct ShaderVariableLayoutsBase;

// This template struct must be specialized for each shader variable struct.
// See VertexShaderInputSet for more info and example.
template<typename ENTRY>
struct ShaderVariableLayouts;

} // vulkan_shaderbuilder_specialization_classes

namespace vulkan::shaderbuilder {
using namespace vulkan_shaderbuilder_specialization_classes;

// This template struct is specialized below for BasicTypeLayout, MemberLayout, ArrayLayout and StructLayout.
// It provides a generic interface to the common parameters of a layout: alignment, size and array_stride.
template<typename XLayout>
struct Layout;

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
  static constexpr utils::TemplateStringLiteral glsl_id_str = GlslIdStr;
};

template<typename ContainingClass, typename XLayout, int Index, size_t MaxAlignment, size_t Offset, utils::TemplateStringLiteral GlslIdStr>
struct Layout<MemberLayout<ContainingClass, XLayout, Index, MaxAlignment, Offset, GlslIdStr>>
{
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
  static constexpr glsl::Standard standard = XLayout::standard;
};

template<typename XLayout, size_t Elements>
struct Layout<ArrayLayout<XLayout, Elements>>
{
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
  static constexpr std::string_view last_member_glsl_id_sv = static_cast<std::string_view>(last_member_layout::glsl_id_str);
  static constexpr size_t underlying_class_name_len = last_member_glsl_id_sv.find(':');
  static constexpr utils::TemplateStringLiteral<underlying_class_name_len> glsl_id_str{last_member_glsl_id_sv.data(), underlying_class_name_len};

  constexpr StructLayout(MembersTuple) {}
};

template<typename MembersTuple>
struct Layout<StructLayout<MembersTuple>>
{
  static constexpr size_t alignment = StructLayout<MembersTuple>::alignment;
  static constexpr size_t size = StructLayout<MembersTuple>::size;
  static constexpr size_t array_stride = std::max({size, alignment, (StructLayout<MembersTuple>::standard == glsl::std140) ? 16 : 0});
};

//
// The type returned by the macro MEMBER.
template<typename ContainingClass, typename XLayout, utils::TemplateStringLiteral MemberName>
struct StructMember
{
  static constexpr size_t alignment = Layout<XLayout>::alignment;
  static constexpr utils::TemplateStringLiteral glsl_id_str = utils::Catenate_v<vulkan_shaderbuilder_specialization_classes::ShaderVariableLayoutsBase<ContainingClass>::prefix, MemberName>;
  using containing_class = ContainingClass;
  using layout_type = XLayout;
};

// Specialization for arrays.
template<typename ContainingClass, typename XLayout, utils::TemplateStringLiteral MemberName, size_t Elements>
struct StructMember<ContainingClass, XLayout[Elements], MemberName>
{
  static constexpr utils::TemplateStringLiteral glsl_id_str = utils::Catenate_v<vulkan_shaderbuilder_specialization_classes::ShaderVariableLayoutsBase<ContainingClass>::prefix, MemberName>;
  static constexpr size_t alignment = Layout<XLayout>::alignment;
  using containing_class = ContainingClass;
  using layout_type = ArrayLayout<XLayout, Elements>;
};

#define LAYOUT_DECLARATION(classname, standard) \
  template<> \
  struct vulkan_shaderbuilder_specialization_classes::ShaderVariableLayoutsBase<classname> \
  { \
    using containing_class = classname; \
    static constexpr std::array long_prefix_a = std::to_array(BOOST_PP_STRINGIZE(classname)"::"); \
    static constexpr std::string_view long_prefix{long_prefix_a.data(), long_prefix_a.size()}; \
    static constexpr std::string_view prefix_sv = long_prefix.substr(long_prefix.find_last_of(':', long_prefix.size() - 4) + 1); \
    static constexpr utils::TemplateStringLiteral<prefix_sv.size()> prefix{prefix_sv.data(), prefix_sv.size()}; \
  }; \
  \
  template<> \
  struct vulkan_shaderbuilder_specialization_classes::ShaderVariableLayouts<classname> : glsl::standard, vulkan_shaderbuilder_specialization_classes::ShaderVariableLayoutsBase<classname>

#define MEMBER(membertype, membername) \
  (::vulkan::shaderbuilder::StructMember<containing_class, membertype, BOOST_PP_STRINGIZE(membername)>{})

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
      FirstUnprocessed::glsl_id_str
    >;

  return make_layouts_impl(std::tuple_cat(layouts, std::tuple<NextLayout>{}), unprocessedMembers...);
}

template<typename FirstMember, typename... RestMembers>
constexpr auto make_struct_layout(FirstMember const& first_member, RestMembers const&... restMembers)
{
  using ContainingClass = typename FirstMember::containing_class;
  using FirstType = typename FirstMember::layout_type;
  using FirstMemberLayout = MemberLayout<ContainingClass, FirstType, 0/*index*/, Layout<FirstType>::alignment, 0/*offset*/, FirstMember::glsl_id_str>;

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
#include "base_types.h"
#undef SHADERBUILDER_STANDARD

#define SHADERBUILDER_STANDARD std140
#include "base_types.h"
#undef SHADERBUILDER_STANDARD

#define SHADERBUILDER_STANDARD std430
#include "base_types.h"
#undef SHADERBUILDER_STANDARD

#define SHADERBUILDER_STANDARD scalar
#include "base_types.h"
#undef SHADERBUILDER_STANDARD

} // namespace standards

} // namespace vulkan::shaderbuilder

// Not sure where to put the 'standards' structs... lets put them in glsl for now.
namespace glsl {

// From https://registry.khronos.org/OpenGL/specs/gl/GLSLangSpec.4.60.html 4.4.5. Uniform and Shader Storage Block Layout Qualifiers:
//
// The std140 and std430 qualifiers override only the packed, shared, std140, and std430 qualifiers; other qualifiers are inherited.
// The std430 qualifier is supported only for shader storage blocks; a shader using the std430 qualifier on a uniform block will
// result in a compile-time error, unless it is also declared with push_constant.
//
// There is an extention that allows to use std430, but we don't use that yet.
struct uniform_std140 : vulkan::shaderbuilder::standards::std140::TypeEncodings
{
  using tag_type = uniform_std140;
};

// From the same paragraph:
//
// However, when push_constant is declared, the default layout of the buffer will be std430. There is no method to globally set this default.
struct push_constant_std430 : vulkan::shaderbuilder::standards::std430::TypeEncodings
{
  using tag_type = push_constant_std430;
};

// The following layouts might require an extension.
#if 1
struct uniform_std430 : vulkan::shaderbuilder::standards::std430::TypeEncodings
{
  using tag_type = uniform_std430;
};

struct uniform_scalar : vulkan::shaderbuilder::standards::scalar::TypeEncodings
{
  using tag_type = uniform_scalar;
};

struct push_constant_scalar : vulkan::shaderbuilder::standards::scalar::TypeEncodings
{
  using tag_type = push_constant_scalar;
};
#endif

} // glsl
