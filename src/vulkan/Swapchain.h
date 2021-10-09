#pragma once

#include "utils/Vector.h"
#include <vulkan/vulkan.hpp>

namespace vulkan {

class PresentationSurface;
class Swapchain;
using SwapchainIndex = utils::VectorIndex<Swapchain>;

class Swapchain
{
 public:
  using images_type = utils::Vector<vk::Image, SwapchainIndex>;
  using image_views_type = utils::Vector<vk::UniqueImageView, SwapchainIndex>;

 private:
  vk::UniqueSwapchainKHR        m_swapchain;
  vk::Format                    m_format;
  images_type                   m_images;
  image_views_type              m_image_views;
  vk::Extent2D                  m_extent;
  vk::PresentModeKHR            m_present_mode;
  vk::ImageUsageFlags           m_usage_flags;
  bool                          m_can_render;

 public:
  Swapchain() : m_format(vk::Format::eUndefined), m_can_render(false) { }

  void prepare(PresentationSurface const& presentation_surface);
};

} // namespace vulkan
