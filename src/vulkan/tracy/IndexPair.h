#pragma once

namespace tracy {

// Mimic a utils::VectorIndex type, while in reality wrapping those of those.
// This was added to be able to use SourceLocationDataIterator as-is.
// It turned out not to be very useful though; it is necessary to see both colors
// belonging to the two wrapped indices. This class is currently not used, might
// delete it later.
struct IndexPair
{
  vulkan::FrameResourceIndex m_frame_resource_index;
  vulkan::SwapchainIndex m_swapchain_index;
  vulkan::SwapchainIndex m_swapchain_index_end;

  IndexPair(vulkan::FrameResourceIndex frame_resource_index, vulkan::SwapchainIndex swapchain_index, vulkan::SwapchainIndex swapchain_index_end) :
    m_frame_resource_index(frame_resource_index), m_swapchain_index(swapchain_index), m_swapchain_index_end(swapchain_index_end) { }

  IndexPair(size_t end) :
    m_frame_resource_index{0}, m_swapchain_index{0}, m_swapchain_index_end(end)
    {
      // Pass the swapchain size.
      ASSERT(end > 0);
    }

  friend bool operator==(IndexPair const& lhs, IndexPair const& rhs)
  {
    // Don't compare apples with oranges.
    ASSERT(lhs.m_swapchain_index_end == rhs.m_swapchain_index_end);
    return lhs.m_frame_resource_index == rhs.m_frame_resource_index &&
           lhs.m_swapchain_index == rhs.m_swapchain_index;
  }

  IndexPair& operator++()
  {
    if (++m_swapchain_index == m_swapchain_index_end)
    {
      m_swapchain_index.set_to_zero();
      ++m_frame_resource_index;
    }
    return *this;
  }

  size_t get_value() const
  {
    return m_frame_resource_index.get_value() * m_swapchain_index_end.get_value() + m_swapchain_index.get_value();
  }

  void set_to_zero()
  {
    m_frame_resource_index.set_to_zero();
    m_swapchain_index.set_to_zero();
  }

  friend std::string to_string(IndexPair const& index_pair)
  {
    return to_string(index_pair.m_frame_resource_index) + "/" + to_string(index_pair.m_swapchain_index);
  }
};

} // namespace tracy
