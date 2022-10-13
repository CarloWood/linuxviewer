#include "sys.h"
#include "shader_builder/ShaderVariableLayouts.h"
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

#if 0
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
#endif

struct Foo;
struct Bar;

LAYOUT_DECLARATION(Foo, uniform_std140)
{
  static constexpr auto struct_layout = make_struct_layout(
    LAYOUT(vec3, m_v3),
    LAYOUT(mat2, m_m2),
    LAYOUT(mat3, m_m3),
    LAYOUT(Float, m_f),
    LAYOUT(dmat4x3, m_dm43)
  );
};

LAYOUT_DECLARATION(Bar, uniform_scalar)
{
  static constexpr auto struct_layout = make_struct_layout(
    LAYOUT(vec3, m_v3),
    LAYOUT(mat2, m_m2),
    LAYOUT(mat3, m_m3),
    LAYOUT(Float, m_f),
    LAYOUT(dmat4x3, m_dm43)
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

  vulkan::shader_builder::ShaderVariableLayouts<Foo> object;
  Dout(dc::notice, libcwd::type_info_of(object.struct_layout).demangled_name());

  vulkan::shader_builder::ShaderVariableLayouts<Bar> object2;
  Dout(dc::notice, libcwd::type_info_of(object2.struct_layout).demangled_name());

  using layout = vulkan::shader_builder::Layout<std::tuple_element_t<1, decltype(object.struct_layout)::members_tuple>::layout_type>;
  static constexpr size_t alignment = layout::alignment;
  static constexpr size_t size = layout::size;
  static constexpr size_t array_stride = layout::array_stride;
  std::cout << "Alignment = " << alignment << "; size = " << size << "; array_stride = " << array_stride << '\n';
}
