#include "sys.h"
#include "RenderPass.h"
#include "SynchronousWindow.h"
#include "LogicalDevice.h"
#include "FrameResourcesData.h"

namespace vulkan {

RenderPass::RenderPass(task::SynchronousWindow* owning_window, std::string const& name) :
  rendergraph::RenderPass(name), m_owning_window(owning_window)
{
  m_owning_window->register_render_pass(this);
}

void RenderPass::create_render_pass()
{
  m_render_pass = m_owning_window->logical_device()->create_render_pass(*this
      COMMA_CWDEBUG_ONLY(vulkan::AmbifixOwner{m_owning_window, "«" + name() + "».m_render_pass"}));
}

void RenderPass::create_imageless_framebuffer(vk::Extent2D extent, uint32_t layers)
{
  m_framebuffer = m_owning_window->logical_device()->create_imageless_framebuffer(*this, extent, layers
      COMMA_CWDEBUG_ONLY(vulkan::AmbifixOwner{m_owning_window, "«" + name() + "».m_framebuffer"}));
  update_framebuffer({{}, extent});
}

std::vector<vk::ClearValue> RenderPass::clear_values() const
{
  DoutEntering(dc::vulkan, "RenderPass::clear_values() [" << name() << "]");
  std::vector<vk::ClearValue> result;

  // Let attachment_nodes list all attachments known to this render pass.
  auto const& attachment_nodes = known_attachments();

  // Run over all known attachments.
  for (auto i = attachment_nodes.ibegin(); i != attachment_nodes.iend(); ++i)
  {
    rendergraph::AttachmentNode const& node = attachment_nodes[i];
    // Paranoia check. We're going to push them in order to result.
    ASSERT(i == node.render_pass_attachment_index());
    rendergraph::Attachment const* attachment = node.attachment();
    Dout(dc::vulkan, i << " : " << attachment->get_clear_value());
    result.push_back(attachment->get_clear_value());
  }

  return result;
}

void RenderPass::prepare_begin_info_chain()
{
  DoutEntering(dc::vulkan, "RenderPass::prepare_begin_info_chain() [" << name() << "]");

  m_clear_values = clear_values();
  m_attachment_image_views.resize(known_attachments().size());  // Will be initialized/updated every frame (rotating frame resources and swapchain images).
  m_begin_info_chain.get<vk::RenderPassBeginInfo>()
    .setRenderPass(*m_render_pass)
    .setClearValues(m_clear_values);
  m_begin_info_chain.get<vk::RenderPassAttachmentBeginInfo>()
    .setAttachments(m_attachment_image_views);
}

void RenderPass::update_image_views(Swapchain const& swapchain, FrameResourcesData const* frame_resources)
{
  DoutEntering(dc::vkframe, "RenderPass::update_image_views(" << frame_resources << ") [" << name() << "]");

  // Let attachment_nodes list all attachments known to this render pass.
  auto const& attachment_nodes = known_attachments();

  // Run over all known attachments and push the corresponding image views of the current frame resources in order to the result vector.
  for (auto i = attachment_nodes.ibegin(); i != attachment_nodes.iend(); ++i)
  {
    rendergraph::AttachmentIndex attachment_index = attachment_nodes[i].attachment()->render_graph_attachment_index();
    vk::ImageView vh_image_view = attachment_index.undefined() ? swapchain.vh_current_image_view() : *frame_resources->m_attachments[attachment_index].m_image_view;
#ifdef CWDEBUG
    vk::Image vh_image = attachment_index.undefined() ? swapchain.images()[swapchain.current_index()] : *frame_resources->m_attachments[attachment_index].m_image;
#endif
    Dout(dc::vkframe, i << " : " << vh_image_view << " (image: " << vh_image << ")");
    m_attachment_image_views[i] = vh_image_view;
  }
}

void RenderPass::update_framebuffer(vk::Rect2D render_area)
{
  DoutEntering(dc::vulkan, "RenderPass::update_framebuffer(" << render_area << ") [" << name() << "]");
  // Usually a framebuffer is (re)created when (also) the extent changed, so update the render_area in one go.
  m_begin_info_chain.get<vk::RenderPassBeginInfo>()
    .setFramebuffer(*m_framebuffer)
    .setRenderArea(render_area);
}

void RenderPass::update_render_area(vk::Rect2D render_area)
{
  DoutEntering(dc::vulkan, "RenderPass::update_render_area(" << render_area << ") [" << name() << "]");
  m_begin_info_chain.get<vk::RenderPassBeginInfo>()
    .setRenderArea(render_area);
}

} // namespace vulkan
