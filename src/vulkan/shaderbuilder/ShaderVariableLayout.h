#pragma once

#include "math/glsl.h"
#include <vulkan/vulkan.hpp>
#include "utils/Vector.h"
#include "utils/log2.h"
#include <boost/preprocessor/stringize.hpp>
#include <functional>
#include <cstring>
#include <iosfwd>
#include <map>
#include <cstdint>
#include "debug.h"

namespace vulkan::pipeline {
class Pipeline;
} // namespace vulkan::pipeline

namespace vulkan::shaderbuilder {

// We need all members to be public because I want to use this as an aggregrate type.
struct BasicType
{
  static constexpr uint32_t standard_width_in_bits = std::bit_width(static_cast<uint32_t>(glsl::number_of_standards - 1));
  static constexpr uint32_t rows_width_in_bits = 3;      // 1, 2, 3 or 4.
  static constexpr uint32_t cols_width_in_bits = 3;      // 1, 2, 3 or 4.
  static_assert(standard_width_in_bits + rows_width_in_bits + cols_width_in_bits == 8, "That doesn't fit");
  static constexpr uint32_t scalar_type_width_in_bits = std::bit_width(static_cast<uint32_t>(glsl::number_of_scalar_types));    // 4 bits
  static constexpr uint32_t log2_alignment_width_in_bits = 8 - scalar_type_width_in_bits;       // Possible alignments are 4, 8, 16 and 32. So 2 bits would be sufficient (we use 3).
  static constexpr uint32_t size_width_in_bits = 8;
  static constexpr uint32_t array_stride_width_in_bits = 8;

  uint32_t       m_standard:standard_width_in_bits;             // See glsl::Standard.
  uint32_t           m_rows:rows_width_in_bits;                 // If rows is 1 then also cols is 1 and this is a Scalar.
  uint32_t           m_cols:cols_width_in_bits;                 // If cols is 1 then this is a Scalar or a Vector.
  uint32_t      m_scalar_type:scalar_type_width_in_bits;            // This type when this is a Scalar, otherwise the underlying scalar type.
  uint32_t m_log2_alignment:log2_alignment_width_in_bits{};     // The log2 of the alignment of this type when not used in an array.
  uint32_t           m_size:size_width_in_bits;                 // The (padded) size of this type when not used in an array.
  uint32_t   m_array_stride:array_stride_width_in_bits{};       // The (padded) size of this type when used in an array (always >= m_size).
                                                                // This size also fullfills the alignment: array_stride % alignment == 0.

 public:
  constexpr glsl::Standard standard() const { return static_cast<glsl::Standard>(m_standard); }
  constexpr int rows() const { return m_rows; }
  constexpr int cols() const { return m_cols; }
  constexpr glsl::Kind kind() const { return (m_rows == 1) ? glsl::Scalar : (m_cols == 1) ? glsl::Vector : glsl::Matrix; }
  constexpr glsl::ScalarIndex scalar_type() const { return static_cast<glsl::ScalarIndex>(m_scalar_type); }
  constexpr int alignment() const { ASSERT(m_standard != glsl::vertex_attributes); return 1 << m_log2_alignment; }
  constexpr int size() const { ASSERT(m_standard != glsl::vertex_attributes); return m_size; }
  constexpr int array_stride() const { ASSERT(m_standard != glsl::vertex_attributes); return m_array_stride; }
  // This applies to Vertex Attributes (aka, see https://registry.khronos.org/OpenGL/specs/gl/GLSLangSpec.4.60.html 4.4.1).
  // An array consumes this per index.
  int consumed_locations() const { ASSERT(standard() == glsl::vertex_attributes); return ((scalar_type() == glsl::eDouble && rows() >= 3) ? 2 : 1) * cols(); }

#if 0
  constexpr operator uint32_t() const
  {
    return (((((m_array_stride <<
              size_width_in_bits | m_size) <<
    log2_alignment_width_in_bits | m_log2_alignment) <<
       scalar_type_width_in_bits | m_scalar_type) <<
              cols_width_in_bits | m_cols) <<
              rows_width_in_bits | m_rows) <<
          standard_width_in_bits | m_standard;
  }
#endif

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

//static_assert(sizeof(BasicType) == sizeof(uint32_t), "Size of BasicType is too large for encoding.");

// This template struct is specialized below for BasicTypeLayout, ArrayLayout and StructLayout.
// It provides a generis interface to the common parameters of a layout: alignment, size and array_stride.
template<typename TypeInfo>
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
template<typename ContainingClass, typename XLayout, int MemberIndex, size_t MaxAlignment, size_t Offset>
struct MemberLayout
{
  using containing_class = ContainingClass;
  using layout_type = XLayout;
  static constexpr int member_index = MemberIndex;
  static constexpr size_t max_alignment = MaxAlignment;
  static constexpr size_t offset = Offset;
};

template<typename ContainingClass, typename XLayout, int Index, size_t MaxAlignment, size_t Offset>
struct Layout<MemberLayout<ContainingClass, XLayout, Index, MaxAlignment, Offset>>
{
  static constexpr size_t alignment = Layout<XLayout>::alignment;
  static constexpr size_t size = Layout<XLayout>::size;
  static constexpr size_t array_stride = Layout<XLayout>::array_stride;
};

// The type returned by the macro MEMBER.
template<typename ContainingClass, typename XLayout>
struct StructMember
{
  using containing_class = ContainingClass;
  using layout_type = XLayout;
  static constexpr size_t alignment = Layout<XLayout>::alignment;
  static constexpr size_t size = Layout<XLayout>::size;

  char const* const m_name;

  constexpr StructMember(char const* name) : m_name(name) { }
};

#define MEMBER(membertype, membername) \
  (::vulkan::shaderbuilder::StructMember<containing_class, membertype>{ BOOST_PP_STRINGIZE(membername) })

//=============================================================================
// Helper classes for constructing StructLayout<> types.
//

// Run up `val` to the next multiple of `factor`.
consteval size_t round_up_to_multiple_off(size_t val, size_t factor)
{
  return (val + factor - 1) - (val + factor - 1) % factor;
}

template<typename PreviousMemberLayout, typename Member>
consteval size_t compute_max_alignment()
{
  return std::max(PreviousMemberLayout::max_alignment, Member::alignment);
}

template<typename PreviousMemberLayout, typename Member>
consteval size_t compute_offset()
{
  return round_up_to_multiple_off(PreviousMemberLayout::offset + Layout<PreviousMemberLayout>::size, Member::alignment);
}

template<typename... MemberLayouts>
constexpr auto make_members_impl(std::tuple<MemberLayouts...> layouts)
{
  return layouts;
}

template<typename... MemberLayouts, typename FirstUnprocessed, typename... RestUnprocessed>
constexpr auto make_members_impl(std::tuple<MemberLayouts...> layouts, FirstUnprocessed const&, RestUnprocessed const&... unprocessedMembers)
{
  using LastMemberLayout = std::tuple_element_t<sizeof...(MemberLayouts) - 1, std::tuple<MemberLayouts...>>;

  using NextLayout = MemberLayout<
      typename FirstUnprocessed::containing_class,
      typename FirstUnprocessed::layout_type,
      sizeof...(MemberLayouts),
      compute_max_alignment<LastMemberLayout, FirstUnprocessed>(),
      compute_offset<LastMemberLayout, FirstUnprocessed>()
    >;

  return make_members_impl(std::tuple_cat(layouts, std::tuple<NextLayout>{}), unprocessedMembers...);
}

constexpr std::tuple<> make_members() { return {}; }

template<typename FirstMember, typename... RestMembers>
constexpr auto make_members(FirstMember const&, RestMembers const&... restMembers)
{
  using ContainingClass = typename FirstMember::containing_class;
  using FirstType = typename FirstMember::layout_type;
  using FirstMemberLayout = MemberLayout<ContainingClass, FirstType, 0/*index*/, Layout<FirstType>::alignment, 0/*offset*/>;

  return make_members_impl(std::make_tuple(FirstMemberLayout{}), restMembers...);
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

struct TypeInfo
{
  std::string name;                             // glsl name
  int const number_of_attribute_indices;        // The number of sequential attribute indices that will be consumed.
  vk::Format const format;                      // The format to use for this type.

  TypeInfo(BasicType type);
};

// Must be an aggregate type.
struct ShaderVariableLayout
{
  BasicType const m_glsl_type;                  // The glsl type of the variable.
  char const* const m_glsl_id_str;              // The glsl name of the variable (unhashed).
  uint32_t const m_offset;                      // The offset of the variable inside its C++ ENTRY struct.
  std::string (*m_declaration)(ShaderVariableLayout const*, pipeline::Pipeline*);    // Pseudo virtual function that generates the declaration.

  std::string name() const;
  std::string declaration(pipeline::Pipeline* pipeline) const
  {
    // Set m_declaration when constructing the derived class. It's like a virtual function.
    ASSERT(m_declaration);
    return m_declaration(this, pipeline);
  }

  // ShaderVariableLayout are put in a boost::ptr_set. Provide a sorting function.
  bool operator<(ShaderVariableLayout const& other) const
  {
    return strcmp(m_glsl_id_str, other.m_glsl_id_str) < 0;
  }

  bool valid_alignment_and_size() const
  {
    auto standard = m_glsl_type.standard();
    return standard == glsl::std140 || standard == glsl::std430 || standard == glsl::scalar;
  }

  uint32_t alignment() const
  {
    return glsl::alignment(m_glsl_type.standard(), m_glsl_type.scalar_type(), m_glsl_type.rows(), m_glsl_type.cols());
  }

  uint32_t size() const
  {
    return glsl::size(m_glsl_type.standard(), m_glsl_type.scalar_type(), m_glsl_type.rows(), m_glsl_type.cols());
  }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::shaderbuilder

// Not sure where to put the 'standards' structs... lets put them in glsl for now.
namespace glsl {

struct per_vertex_data : vulkan::shaderbuilder::standards::vertex_attributes::TypeEncodings
{
  using tag_type = per_vertex_data;
  static constexpr vk::VertexInputRate input_rate = vk::VertexInputRate::eVertex;       // This is per vertex data.
};

struct per_instance_data : vulkan::shaderbuilder::standards::vertex_attributes::TypeEncodings
{
  using tag_type = per_instance_data;
  static constexpr vk::VertexInputRate input_rate = vk::VertexInputRate::eInstance;     // This is per instance data.
};

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
