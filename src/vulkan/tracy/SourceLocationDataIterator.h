#pragma once

#include "FrameResourceIndex.h"
#include "SwapchainIndex.h"
#include <server/TracyEvent.hpp>
#include <client/TracyProfiler.hpp>
#include <iterator>
#include <concepts>
#include <type_traits>
#include <cstddef>      // std::ptrdiff_t
#include <string>       // to_string
#include <cstring>      // strdup
#include "debug.h"

namespace tracy {

template<typename T>
concept ConceptTracyIndex = std::is_same_v<T, vulkan::FrameResourceIndex> || std::is_same_v<T, vulkan::SwapchainIndex>;

template<ConceptTracyIndex TracyIndex>
struct SourceLocationDataIterator
{
  using difference_type = std::ptrdiff_t;
  using value_type = SourceLocationData;

  // Construct end().
  constexpr SourceLocationDataIterator(TracyIndex max_index) : m_index(max_index) { }

  // Construct begin().
  constexpr SourceLocationDataIterator(std::string const& base_name, char const* function, char const* file, uint32_t line, uint32_t color) :
    m_source_location{ .name = base_name.c_str(), .function = function, .file = file, .line = line, .color = color },
    m_index{0} { }

  SourceLocationDataIterator& operator++() { ++m_index; return *this; }
  SourceLocationDataIterator operator++(int) { ASSERT(false); return TracyIndex{}; }

  value_type operator*() const {
    // This leaks memory. Each iterator should only be dereferenced once per index value.
    return { .name = strdup((std::string(m_source_location.name) + " " + to_string(m_index)).c_str()),
             .function = m_source_location.function,
             .file = m_source_location.file,
             .line = m_source_location.line,
             .color = m_source_location.color };
  }

  friend bool operator==(SourceLocationDataIterator const& lhs, SourceLocationDataIterator const& rhs)
  {
    return lhs.m_index == rhs.m_index;
  }

 private:
  SourceLocationData m_source_location{};
  TracyIndex m_index;
};

static_assert(std::input_iterator<SourceLocationDataIterator<vulkan::FrameResourceIndex>>);
static_assert(std::input_iterator<SourceLocationDataIterator<vulkan::SwapchainIndex>>);

} // namespace tracy

