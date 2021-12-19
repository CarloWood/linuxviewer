#include "sys.h"
#include "AttachmentNode.h"
#include "RenderPass.h"
#include "debug.h"

namespace vulkan::rendergraph {

#ifdef CWDEBUG
void AttachmentNode::print_on(std::ostream& os) const
{
  os << m_render_pass << '/' << m_attachment << ' ' << m_index;
}
#endif

void AttachmentNode::set_load()
{
  DoutEntering(dc::renderpass, "AttachmentNode::set_load() [" << m_render_pass << "/" << m_attachment << "]");
  try
  {
    m_ops.set_load();
  }
  catch (AIAlert::Error const& error)
  {
    THROW_ALERT("Attachment \"[ATTACHMENT]\", render pass \"[RENDERPASS]\"", AIArgs("[ATTACHMENT]", m_attachment)("[RENDERPASS]", m_render_pass), error);
  }
}

void AttachmentNode::set_clear()
{
  DoutEntering(dc::renderpass, "AttachmentNode::set_clear() [" << m_render_pass << "/" << m_attachment << "]");
  try
  {
    m_ops.set_clear();
  }
  catch (AIAlert::Error const& error)
  {
    THROW_ALERT("Attachment \"[ATTACHMENT]\", render pass \"[RENDERPASS]\"", AIArgs("[ATTACHMENT]", m_attachment)("[RENDERPASS]", m_render_pass), error);
  }
}

void AttachmentNode::set_store()
{
  DoutEntering(dc::renderpass, "AttachmentNode::set_store() [" << m_render_pass << "/" << m_attachment << "]");
  m_ops.set_store();
}

void AttachmentNode::set_preserve()
{
  DoutEntering(dc::renderpass, "AttachmentNode::set_preserve() [" << m_render_pass << "/" << m_attachment << "]");
  m_ops.set_preserve();
}

} // namespace vulkan::rendergraph
