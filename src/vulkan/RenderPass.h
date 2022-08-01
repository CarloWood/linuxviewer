#pragma once

#include "rendergraph/RenderPass.h"

namespace vulkan {

class Swapchain;
class FrameResourcesData;

class RenderPass : public rendergraph::RenderPass
{
 private:
  task::SynchronousWindow* m_owning_window;
  vk::UniqueRenderPass m_render_pass;           // The underlying Vulkan render pass.
  vk::UniqueFramebuffer m_framebuffer;          // The imageless framebuffer used by this render pass.

  // initialize_begin_info_chain():
  std::vector<vk::ClearValue> m_clear_values;                                                   // Pointed to by m_begin_info_chain.
  utils::Vector<vk::ImageView, rendergraph::pAttachmentsIndex> m_attachment_image_views;        // Pointed to by m_begin_info_chain
  vk::StructureChain<vk::RenderPassBeginInfo, vk::RenderPassAttachmentBeginInfo> m_begin_info_chain;

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

  // Prepare m_begin_info_chain.
  void prepare_begin_info_chain();

  // Return a vector with image views for this RenderPass that
  // can be used for vk::RenderPassAttachmentBeginInfo::pAttachments, for use with imageless frame buffers.
  void update_image_views(Swapchain const& swapchain, FrameResourcesData const* frame_resources);

  // Update the framebuffer handle and render area that this render pass uses.
  void update_framebuffer(vk::Rect2D render_area);

  // Update the render area that this render pass renders into.
  void update_render_area(vk::Rect2D render_area);

  // Accessors.
  vk::RenderPass vh_render_pass() const
  {
    return *m_render_pass;
  }

  vk::Framebuffer vh_framebuffer() const
  {
    return *m_framebuffer;
  }

  vk::RenderPassBeginInfo const& begin_info() const
  {
    return m_begin_info_chain.get<vk::RenderPassBeginInfo>();
  }

 private:
  // Return a vector with clear values for this RenderPass that
  // can be used for vk::RenderPassBeginInfo::pClearValues.
  std::vector<vk::ClearValue> clear_values() const;

  void create_render_pass() override;
};

} // namespace vulkan
