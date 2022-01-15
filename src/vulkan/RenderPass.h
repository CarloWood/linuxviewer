#pragma once

#include "rendergraph/RenderPass.h"

namespace vulkan {

class Swapchain;
class FrameResourcesData;

class RenderPass : public rendergraph::RenderPass
{
 private:
  task::SynchronousWindow* m_owning_window;
  vk::UniqueRenderPass m_render_pass;           // The underlaying Vulkan render pass.
  vk::UniqueFramebuffer m_framebuffer;          // The imageless framebuffer used by this render pass.

 public:
  // Constructor (only construct RenderPass nodes as objects in your Window class.
  //
  // For example, use:
  //
  // class MyWindow : public task::SynchronousWindow {
  //   RenderPass main_pass{this, "main_pass"};
  //
  RenderPass(task::SynchronousWindow* owning_window, std::string const& name);

  // Called from rendergraph::generate after creating the render pass.
  void set_render_pass(vk::UniqueRenderPass&& render_pass) { m_render_pass = std::move(render_pass); }

  // Called from SynchronousWindow::recreate_framebuffers.
  void create_imageless_framebuffer(vk::Extent2D extent, uint32_t layers);

  // Return a vector with clear values for this RenderPass that
  // can be used for vk::RenderPassBeginInfo::pClearValues.
  std::vector<vk::ClearValue> clear_values() const;

  // Return a vector with image views for this RenderPass that
  // can be used for vk::RenderPassAttachmentBeginInfo::pAttachments, for use with imageless frame buffers.
  std::vector<vk::ImageView> attachment_image_views(Swapchain const& swapchain, FrameResourcesData const* frame_resources) const;

  // Accessors.
  vk::RenderPass vh_render_pass() const
  {
    return *m_render_pass;
  }

  vk::Framebuffer vh_framebuffer() const
  {
    return *m_framebuffer;
  }

 private:
  void create_render_pass() override;
};

} // namespace vulkan
