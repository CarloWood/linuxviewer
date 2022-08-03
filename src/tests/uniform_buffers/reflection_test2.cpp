#include "sys.h"
#include "shaderbuilder/ShaderVariableLayouts.h"
#include <iostream>
#include <type_traits>
#include <numeric>
#include "debug.h"

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
concept ConceptBaseType = std::is_same_v<T, float> || std::is_same_v<T, double> || std::is_same_v<T, bool> || std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t>;

// Note: this type is only used for declaration; so conversion to T upon using it in any way is perfectly OK.
template<ConceptBaseType T>
struct Builtin
{
  alignas(std::is_same_v<T, double> ? 8 : 4) T m_val;

  Builtin(T val = {}) : m_val(val) { }
  operator T() const { return m_val; }
  operator T&() { return m_val; }
};

template<typename T>
struct WrappedType : T
{
};

template<ConceptBaseType T>
struct WrappedType<T> : Builtin<T>
{
};

template<typename T, size_t alignment, size_t size>
struct AlignAndResize : WrappedType<T>
{
  static constexpr size_t required_alignment = alignment;
  // In C++ the size must always be a multiple to the alignment. Therefore, we have to possibly relax
  // the (C++ enforced) alignment or the resulting size will be too large!
  // In order to get the right alignment this that case, padding must be added before WrappedType<T>,
  // which will depend on the previous elements.
  static constexpr size_t enforced_alignment = std::gcd(required_alignment, size);
  static constexpr size_t rounded_up_size = (sizeof(T) + enforced_alignment - 1) / enforced_alignment * enforced_alignment;
  static_assert(size >= rounded_up_size, "You can't request a size that is less than the actual size of T.");
  static constexpr size_t required_padding = size - rounded_up_size;

  [[no_unique_address]] AlignedPadding<required_padding, enforced_alignment> __padding;
};

// Define test basic types.
constexpr glsl::Standard st = glsl::std140;

using Float = AlignAndResize<glsl::Float, glsl::alignment(st, glsl::eFloat, 1, 1), glsl::size(st, glsl::eFloat, 1, 1)>;
using vec3  = AlignAndResize< glsl::vec3, glsl::alignment(st, glsl::eFloat, 3, 1), glsl::size(st, glsl::eFloat, 3, 1)>;
using mat2  = AlignAndResize< glsl::mat2, glsl::alignment(st, glsl::eFloat, 2, 2), glsl::size(st, glsl::eFloat, 2, 2)>;
using mat3  = AlignAndResize< glsl::mat3, glsl::alignment(st, glsl::eFloat, 3, 3), glsl::size(st, glsl::eFloat, 3, 3)>;

template<typename T>
concept ConceptAlignAndResize = requires(T x) {
  { AlignAndResize{x} } -> std::same_as<T>;
};

template<typename ContainingClass, typename T>
struct Member
{
  using containing_class = ContainingClass;
  using type = T;
  static constexpr size_t alignment = vulkan::shaderbuilder::standards::Layout<T>::alignment;
  static constexpr size_t size = vulkan::shaderbuilder::standards::Layout<T>::size;

  char const* const m_name;

  constexpr Member(char const* name) : m_name(name) { }
};

template<typename ContainingClass, typename T, int index_, size_t alignment_, size_t size_, size_t offset_>
struct MemberLayout
{
  using containing_class = ContainingClass;
  using type = T;
  static constexpr int index = index_;
  static constexpr size_t alignment = alignment_;
  static constexpr size_t size = size_;
  static constexpr size_t offset = offset_;
};

template<typename PreviousLayout, typename Member>
consteval size_t compute_alignment()
{
  return Member::alignment;
}

template<typename PreviousLayout, typename Member>
consteval size_t compute_size()
{
  return Member::size;
}

consteval size_t round_up_to_multiple_off(size_t val, size_t factor)
{
  return (val + factor - 1) - (val + factor - 1) % factor;
}

template<typename PreviousLayout, typename Member>
consteval size_t compute_offset()
{
  return round_up_to_multiple_off(PreviousLayout::offset + PreviousLayout::size, Member::alignment);
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
      typename FirstUnprocessed::type,
      sizeof...(MemberLayouts),
      compute_alignment<LastMemberLayout, FirstUnprocessed>(),
      compute_size<LastMemberLayout, FirstUnprocessed>(),
      compute_offset<LastMemberLayout, FirstUnprocessed>()
    >;

  return make_members_impl(std::tuple_cat(layouts, std::tuple<NextLayout>{}), unprocessedMembers...);
}

constexpr std::tuple<> make_members() { return {}; }

template<typename FirstMember, typename... RestMembers>
constexpr auto make_members(FirstMember const&, RestMembers const&... restMembers)
{
  using ContainingClass = typename FirstMember::containing_class;
  using FirstType = typename FirstMember::type;
  using FirstMemberLayout = MemberLayout<ContainingClass, FirstType, 0, vulkan::shaderbuilder::standards::Layout<FirstType>::alignment, vulkan::shaderbuilder::standards::Layout<FirstType>::size, 0>;

  return make_members_impl(std::make_tuple(FirstMemberLayout{}), restMembers...);
}

#define STRINGIFY(x) STR(x)
#define STR(x) #x

#define MEMBER(membertype, membername) \
  Member<containing_class, membertype>{ STRINGIFY(membername) }

struct Foo;
struct Bar;

template<>
struct vulkan::shaderbuilder::ShaderVariableLayouts<Foo> : glsl::uniform_std140
{
  using containing_class = Foo;
  static constexpr auto members = make_members(
    MEMBER(Float, m_f),
    MEMBER(vec3, m_v3),
    MEMBER(mat2, m_m2),
    MEMBER(mat3, m_m3),
    MEMBER(dmat4x3, m_dm43)
  );
};

template<>
struct vulkan::shaderbuilder::ShaderVariableLayouts<Bar> : glsl::uniform_scalar
{
  using containing_class = Bar;
  static constexpr auto members = make_members(
    MEMBER(Float, m_f),
    MEMBER(vec3, m_v3),
    MEMBER(mat2, m_m2),
    MEMBER(mat3, m_m3),
    MEMBER(dmat4x3, m_dm43)
  );
};

#if 0
struct Foo : glsl::uniform_std140
{
  ::Float m_f;
  ::vec3  m_v3;
  ::mat2  m_m2;
  ::mat3  m_m3;
};
#endif

int main()
{
  Debug(NAMESPACE_DEBUG::init());

  vulkan::shaderbuilder::ShaderVariableLayouts<Foo> object;
  Dout(dc::notice, libcwd::type_info_of(object.members).demangled_name());

  vulkan::shaderbuilder::ShaderVariableLayouts<Bar> object2;
  Dout(dc::notice, libcwd::type_info_of(object2.members).demangled_name());

  using test_type = vulkan::shaderbuilder::standards::Layout<std::tuple_element_t<1, decltype(object.members)>::type>;
  static constexpr size_t alignment = test_type::alignment;
  static constexpr size_t size = test_type::size;
  static constexpr size_t array_stride = test_type::array_stride;
  std::cout << "Alignment = " << alignment << "; size = " << size << "; array_stride = " << array_stride << '\n';
}
