#include "sys.h"
#include "vk_enums_reflection.h"
#include <vulkan/vulkan.hpp>
#include <string_view>
#include <array>
#include <cstring>
#include <concepts>

namespace {

template<typename EnumType>
struct Element
{
  EnumType value;
  std::string_view name;
};

constexpr int count_arguments(std::string_view str)
{
  int count = str.empty() ? 0 : 1;
  for (char c : str)
    if (c == ',')
      ++count;
  return count;
}

constexpr std::string_view pop_str(std::string_view& str)
{
  std::string_view ret;
  std::size_t comma = str.find(',');
  if (comma == std::string_view::npos)
  {
    ret = str;
    str = "";
    return ret;
  }

  ret = str.substr(0, comma);
  str = str.substr(comma + 1);
  std::size_t non_space = str.find_first_not_of(' ');
  if (non_space == std::string_view::npos)
    str = "";
  else
    str = str.substr(non_space);

  return ret;
}

template<typename EnumType, int count>
struct EnumHelper
{
  std::string_view m_elements_str;

  constexpr EnumHelper(std::string_view str) : m_elements_str(str) { }

  template<typename ...Args>
  constexpr std::array<Element<EnumType>, count> operator()(Args... args)
  {
    std::array<Element<EnumType>, count> ret{};
    int index = 0;
    ((ret[index].value = args, ret[index++].name = pop_str(m_elements_str)), ...);
    return ret;
  }
};

template<typename EnumType, int count>
constexpr EnumHelper<EnumType, count> initialize_elements(std::string_view str)
{
  return {str};
}

} // namespace

using namespace vk;

#define ENUM_DECLARATION(EnumType, EnumUnderlyingType, ...) \
template<> \
struct Table<vk::EnumType> \
{ \
  static constexpr int count = count_arguments(#__VA_ARGS__); \
  static constexpr std::array<Element<vk::EnumType>, count> elements = \
    initialize_elements<vk::EnumType, count>(#__VA_ARGS__)(__VA_ARGS__); \
};

namespace vk_enums {

template<typename ENUM>
struct Table;

#include <debug/vk_enums.h>     // Generated file.

template<typename EnumType>
[[nodiscard]] auto enum_name(EnumType value) noexcept -> detail::enable_if_enum_t<EnumType, std::string_view>
{
  for (int i = 0; i < Table<EnumType>::count; ++i)
    if (Table<EnumType>::elements[i].value == value)
      return Table<EnumType>::elements[i].name;

  return {};
}

template<typename EnumType>
[[nodiscard]] auto enum_cast(std::string_view name) noexcept -> detail::enable_if_enum_t<EnumType, std::optional<std::decay_t<EnumType>>>
{
  for (int i = 0; i < Table<EnumType>::count; ++i)
    if (Table<EnumType>::elements[i].name == name)
      return Table<EnumType>::elements[i].value;

  return {};
}

// Instantiate the template functions enum_name and enum_cast.
#undef ENUM_DECLARATION
#define ENUM_DECLARATION(EnumType, ...) \
    template std::string_view enum_name(vk::EnumType value) noexcept; \
    template std::optional<vk::EnumType> enum_cast<vk::EnumType>(std::string_view value) noexcept;

#include <debug/vk_enums.h>

} // namespace vk_enums
