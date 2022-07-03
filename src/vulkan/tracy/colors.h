#pragma once

#include "FrameResourceIndex.h"
#include "SwapchainIndex.h"
#include "IndexPair.h"
#include <concepts>
#include <type_traits>
#include "debug.h"

namespace tracy {

template<typename T>
concept ConceptTracyIndex = std::is_same_v<T, vulkan::FrameResourceIndex> || std::is_same_v<T, vulkan::SwapchainIndex> || std::is_same_v<T, tracy::IndexPair>;

static constexpr int max_color = 87;
extern int colors[max_color];

inline int color(int index)
{
  ASSERT(0 <= index && index < max_color);
  return colors[index];
}

// Convert a zone name into a color.
extern int get_color(uint64_t name);

template<ConceptTracyIndex TracyIndex>
int get_color(TracyIndex const& index)
{
  // Return a different value based on type and value.
  static constexpr uint64_t type =
    std::is_same_v<TracyIndex, vulkan::FrameResourceIndex> ? 0x1000 :
    std::is_same_v<TracyIndex, vulkan::SwapchainIndex> ? 0x2000 :
    0x3000;

  return get_color(type + index.get_value());
}

} // namespace tracy
