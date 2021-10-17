#pragma once

#include <vulkan/vulkan.hpp>
#include <magic_enum.hpp>
#include <boost/preprocessor/list/for_each.hpp>
#include <boost/preprocessor/variadic/to_list.hpp>
#include <type_traits>

#define VULKAN_QUEUE_FLAGS_DECLARE_BASE_ELEMENT(n, Type, name) \
  name = static_cast<std::underlying_type_t<Type>>(Type::name),

#define VULKAN_QUEUE_FLAGS_BASE_ELEMENTS(Type, ...) \
  BOOST_PP_LIST_FOR_EACH(VULKAN_QUEUE_FLAGS_DECLARE_BASE_ELEMENT, Type, BOOST_PP_VARIADIC_TO_LIST(__VA_ARGS__))

namespace vulkan {

template<typename BaseEnum>
struct Base
{
  static constexpr size_t count = magic_enum::enum_count<BaseEnum>();
  static constexpr BaseEnum last = magic_enum::enum_value<BaseEnum>(count - 1);
  static constexpr std::underlying_type_t<BaseEnum> last_value = magic_enum::enum_integer(last);
};

// Define enum class vulkan::QueueFlagBits that is extended with ePresentation.
enum class QueueFlagBits : VkQueueFlags
{
  none = 0,

  VULKAN_QUEUE_FLAGS_BASE_ELEMENTS(vk::QueueFlagBits,
    eGraphics, eCompute, eTransfer, eSparseBinding, eProtected)

  ePresentation = Base<vk::QueueFlagBits>::last_value << 1
};

} // namespace vulkan

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
  QueueFlags(vk::QueueFlags queue_flags) : vk::Flags<QueueFlagBits>(static_cast<MaskType>(queue_flags)) { }

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
