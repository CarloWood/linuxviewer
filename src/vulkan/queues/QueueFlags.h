#pragma once

#include <vulkan/vulkan.hpp>
#include <enchantum/enchantum.hpp>
#include <boost/preprocessor/list/for_each.hpp>
#include <boost/preprocessor/variadic/to_list.hpp>
#include <cstddef>
#include <type_traits>

#define VULKAN_QUEUE_FLAGS_DECLARE_BASE_ELEMENT(n, Type, name) \
  name = static_cast<std::underlying_type_t<Type>>(Type::name),

#define VULKAN_QUEUE_FLAGS_BASE_ELEMENTS(Type, ...) \
  BOOST_PP_LIST_FOR_EACH(VULKAN_QUEUE_FLAGS_DECLARE_BASE_ELEMENT, Type, BOOST_PP_VARIADIC_TO_LIST(__VA_ARGS__))

namespace vulkan {

template<typename BaseEnum>
struct QueueFlagBitsBase
{
  static constexpr std::size_t count = enchantum::count<BaseEnum>;
  static constexpr BaseEnum last = enchantum::values<BaseEnum>[count - 1];
  static constexpr std::underlying_type_t<BaseEnum> last_value = enchantum::to_underlying(last);
};

// Define enum class vulkan::QueueFlagBits that is extended with ePresentation.
enum class QueueFlagBits : VkQueueFlags
{
  none = 0,

  VULKAN_QUEUE_FLAGS_BASE_ELEMENTS(vk::QueueFlagBits,
    eGraphics, eCompute, eTransfer, eSparseBinding, eProtected)

  ePresentation = QueueFlagBitsBase<vk::QueueFlagBits>::last_value << 1
};

} // namespace vulkan

// Limit reflection of vulkan::QueueFlagBits to the explicit queue flag values.
// This avoids enchantum's missing-value check looking past the synthetic ePresentation bit.
template<>
struct enchantum::enum_traits<vulkan::QueueFlagBits>
{
  static constexpr auto min = 0;
  static constexpr auto max = static_cast<std::underlying_type_t<vulkan::QueueFlagBits>>(vulkan::QueueFlagBits::ePresentation);
};

namespace vk {

// Specialization of vk::FlagTraits<vulkan::QueueFlagBits>
// to include ePresentation in allFlags.
template <>
struct FlagTraits<vulkan::QueueFlagBits>
{
  enum : VkFlags
  {
    allFlags = VkFlags(FlagTraits<vk::QueueFlagBits>::allFlags) |
               VkFlags(vulkan::QueueFlagBits::ePresentation)
  };
};

} // namespace vk

namespace vulkan {

class QueueFlags : public vk::Flags<QueueFlagBits>
{
 public:
  using vk::Flags<QueueFlagBits>::Flags;

  // Provide a converter from vk::QueueFlags to this extension.
  constexpr QueueFlags(vk::QueueFlags queue_flags) : vk::Flags<QueueFlagBits>(static_cast<MaskType>(queue_flags)) { }

  // Unfortunately, we have to repeat all operators that return the type of this class.
  // Moreover, vk::Flags::m_mask is private (instead of protected) so we need to use
  // `operator MaskType` to read it and `operator=` to write to it.

  // Bitwise operators.
  constexpr QueueFlags operator&(QueueFlags const& rhs) const noexcept
  {
    return QueueFlags(operator MaskType() & rhs.operator MaskType());
  }

  constexpr QueueFlags operator|(QueueFlags const& rhs) const noexcept
  {
    return QueueFlags(operator MaskType() | rhs.operator MaskType());
  }

  constexpr QueueFlags operator^(QueueFlags const& rhs) const noexcept
  {
    return QueueFlags(operator MaskType() ^ rhs.operator MaskType());
  }

  constexpr QueueFlags operator~() const noexcept
  {
    return QueueFlags(operator MaskType() ^ vk::FlagTraits<QueueFlagBits>::allFlags);
  }

  // Assignment operators.
  constexpr QueueFlags& operator=(QueueFlags const& rhs) noexcept = default;

  constexpr QueueFlags& operator|=(QueueFlags const& rhs) noexcept
  {
    return operator=(*this | rhs);
  }

  constexpr QueueFlags& operator|=(vk::QueueFlags const& rhs) noexcept
  {
    return operator=(*this | rhs);
  }

  constexpr QueueFlags& operator&=(QueueFlags const& rhs) noexcept
  {
    return operator=(*this & rhs);
  }

  constexpr QueueFlags& operator^=(QueueFlags const& rhs) noexcept
  {
    return operator=(*this ^ rhs);
  }

  void print_on(std::ostream& os) const;
};

inline QueueFlags operator|(QueueFlagBits lhs, QueueFlagBits rhs)
{
  return QueueFlags(lhs) | rhs;
}

inline QueueFlags operator|(vk::QueueFlags lhs, QueueFlagBits rhs)
{
  return QueueFlags(lhs) | rhs;
}

} // namespace vulkan

#undef VULKAN_QUEUE_FLAGS_BASE_ELEMENTS
#undef VULKAN_QUEUE_FLAGS_DECLARE_BASE_ELEMENT
