#include "sys.h"
#include "RenderPass.h"
#include "utils/AIAlert.h"

namespace vulkan::rendergraph {

RenderPass& RenderPass::operator[](Attachment::OpRemoveOrDontCare mod_attachment)
{
  Dout(dc::renderpass, this << "[" << mod_attachment << "]");
  Attachment const* attachment = mod_attachment.m_attachment;

#ifdef CWDEBUG
  // Search if attachment is already know.
  auto known_attachment = find_by_ID(m_known_attachments, attachment);
  if (known_attachment != m_known_attachments.end())
    THROW_ALERT("Trying to remove attachment with \"[-[ATTACHMENT]]\" after first adding it, in render pass \"[RENDERPASS]\".", AIArgs("[ATTACHMENT]", attachment)("[RENDERPASS]", this));
  // Search if attachment was already removed.
  auto remove_or_dontcare_attachment = find_by_ID(m_remove_or_dontcare_attachments, attachment);
  if (remove_or_dontcare_attachment != m_remove_or_dontcare_attachments.end())
    THROW_ALERT("Can't remove an attachment twice (\"[-[ATTACHMENT]]\" in render pass \"[RENDERPASS]\").", AIArgs("[ATTACHMENT]", attachment)("[RENDERPASS]", this));
#endif

  // Add attachment to m_remove_or_dontcare_attachments.
  // This will cause that attachment to not be added as input when the preceding render pass stores it, or,
  // when it doesn't, mark the attachment as a LOAD_OP_DONTCARE.
  m_remove_or_dontcare_attachments.push_back(attachment);

  return *this;
}

RenderPass& RenderPass::operator[](Attachment::OpLoad mod_attachment)
{
  Dout(dc::renderpass, this << "[" << mod_attachment << "]");
  AttachmentNode& node = get_node(mod_attachment.m_attachment);
  node.set_load();
  return *this;
}

RenderPass& RenderPass::operator[](Attachment::OpClear mod_attachment)
{
  Dout(dc::renderpass, this << "[" << mod_attachment << "]");
  AttachmentNode& node = get_node(mod_attachment.m_attachment);
  node.set_clear();
  return *this;
}

void RenderPass::do_load_dont_cares()
{
  // Each attachment that is still listed in m_remove_or_dontcare_attachments wasn't removed, so is a dontcare.
  std::vector<Attachment const*> dontcare_attachments;
  dontcare_attachments.swap(m_remove_or_dontcare_attachments);
  for (Attachment const* attachment : dontcare_attachments)
  {
    // Just add them to the list of known attachments.
    [[maybe_unused]] AttachmentNode& node = get_node(attachment);
  }
}

void RenderPass::store_attachment(Attachment const& attachment)
{
  AttachmentNode& node = get_node(&attachment);
  if (node.is_clear())
    THROW_ALERT("Attachment \"[OUTPUT]\" already specified as input. Put the CLEAR (~) in the stores() of render pass \"[RENDERPASS]\".",
        AIArgs("[OUTPUT]", attachment)("[RENDERPASS]", this));
  node.set_store();
}

void RenderPass::store_attachment(Attachment::OpClear mod_attachment)
{
  AttachmentNode& node = get_node(mod_attachment.m_attachment);
  node.set_store();
  node.set_clear();
}

AttachmentNode& RenderPass::get_node(Attachment const* attachment)
{
  // Search if attachment is already know.
  auto iter = find_by_ID(m_known_attachments, attachment);

  // If already known, return it.
  if (iter != m_known_attachments.end())
    return *iter;

  // It is not allowed to add an attachment and remove at the same time.
  auto removed = find_by_ID(m_remove_or_dontcare_attachments, attachment);
  if (removed != m_remove_or_dontcare_attachments.end())
    THROW_ALERT("Can't add (load, clear or store) an attachment that is explicitly removed with [-[ATTACHMENT]]", AIArgs("[ATTACHMENT]", attachment));

  // Construct a new node.
  Dout(dc::renderpass, "Assigning index " << m_next_index << " to attachment \"" << attachment << "\" of render pass \"" << this << "\".");

  // Store the new node in m_known_attachments.
  return m_known_attachments.emplace_back(this, attachment, m_next_index++);
}

bool RenderPass::is_known(Attachment const& attachment) const
{
  for (AttachmentNode const& node : m_known_attachments)
    if (node.id() == attachment.id())
      return true;
  return false;
}

void RenderPass::preceding_render_pass_stores(Attachment const* attachment)
{
  DoutEntering(dc::renderpass, "RenderPass::preceding_render_pass_stores(" << attachment << ")");

  // Search if this node should be ignored.
  auto iter = find_by_ID(m_remove_or_dontcare_attachments, attachment);
  if (iter != m_remove_or_dontcare_attachments.end())
  {
    // Should be ignored.
    m_remove_or_dontcare_attachments.erase(iter);
    return;
  }

  // Mark this attachment as an attachment that should be loaded.
  get_node(attachment).set_load();
}

vk::AttachmentLoadOp RenderPass::get_load_op(Attachment const& attachment) const
{
  auto node = find_by_ID(m_known_attachments, &attachment);
  if (node == m_known_attachments.end())
    return vk::AttachmentLoadOp::eNoneEXT;
  if (node->is_load())
    return vk::AttachmentLoadOp::eLoad;
  if (node->is_clear())
    return vk::AttachmentLoadOp::eClear;
  return vk::AttachmentLoadOp::eDontCare;
}

vk::AttachmentStoreOp RenderPass::get_store_op(Attachment const& attachment) const
{
  auto node = find_by_ID(m_known_attachments, &attachment);
  if (node == m_known_attachments.end())
    return vk::AttachmentStoreOp::eNoneEXT;
  if (node->is_store())
    return vk::AttachmentStoreOp::eStore;
  return vk::AttachmentStoreOp::eDontCare;
}

#ifdef CWDEBUG
void RenderPass::print_on(std::ostream& os) const
{
  os << m_name;
}
#endif

} // namespace vulkan::rendergraph
