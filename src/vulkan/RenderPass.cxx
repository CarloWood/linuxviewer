#include "sys.h"
#include "RenderPass.h"
#include "SynchronousWindow.h"
#include "LogicalDevice.h"

namespace vulkan {

RenderPass::RenderPass(task::SynchronousWindow* owning_window, std::string const& name) :
  rendergraph::RenderPass(name), m_owning_window(owning_window)
{
  m_owning_window->register_render_pass(this);
}

void RenderPass::create_render_pass()
{
  m_render_pass = m_owning_window->logical_device().create_render_pass(*this
      COMMA_CWDEBUG_ONLY(vulkan::AmbifixOwner{m_owning_window, "«" + name() + "».m_render_pass"}));
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
    ASSERT(i == node.index());
    // All rendergraph::Attachment objects are (base classes of) vulkan::Attachment's.
    Attachment const* attachment = static_cast<Attachment const*>(node.attachment());
    Dout(dc::vulkan, i << " : " << attachment->get_clear_value());
    result.push_back(attachment->get_clear_value());
  }

  return result;
}

} // namespace vulkan
