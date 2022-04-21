#include "sys.h"
#include "RenderPass.h"
#include "SynchronousWindow.h"
#include "LogicalDevice.h"
#include "utils/AIAlert.h"
#ifdef CWDEBUG
#include "debug_ostream_operators.h"
#endif

namespace vulkan::rendergraph {

RenderPass& RenderPass::operator[](Attachment::OpRemoveOrDontCare mod_attachment)
{
  Dout(dc::renderpass, this << "[" << mod_attachment << "]");
  Attachment const* attachment = mod_attachment.m_attachment;

  // Make sure that attachment has an index.
  if (attachment->undefined_index())
    attachment->assign_unique_index();

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
  // If we get here for 'attachment' it is used and we assign a unique ID.
  if (attachment->undefined_index())
    attachment->assign_unique_index();

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

bool RenderPass::is_known(Attachment const* attachment) const
{
  for (AttachmentNode const& node : m_known_attachments)
    if (node.render_graph_attachment_index() == attachment->render_graph_attachment_index())
      return true;
  return false;
}

bool RenderPass::is_load(Attachment const* attachment) const
{
  for (AttachmentNode const& node : m_known_attachments)
    if (node.render_graph_attachment_index() == attachment->render_graph_attachment_index())
      return node.is_load();
  return false;
}

bool RenderPass::is_clear(Attachment const* attachment) const
{
  for (AttachmentNode const& node : m_known_attachments)
    if (node.render_graph_attachment_index() == attachment->render_graph_attachment_index())
      return node.is_clear();
  return false;
}

bool RenderPass::is_store(Attachment const* attachment) const
{
  for (AttachmentNode const& node : m_known_attachments)
    if (node.render_graph_attachment_index() == attachment->render_graph_attachment_index())
      return node.is_store();
  return false;
}

void RenderPass::preceding_render_pass_stores(Attachment const* attachment)
{
  DoutEntering(dc::renderpass, "RenderPass::preceding_render_pass_stores(" << attachment << ") [" << this << "]");

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

vk::AttachmentLoadOp RenderPass::get_load_op(Attachment const* attachment) const
{
  auto node = find_by_ID(m_known_attachments, attachment);
  if (node == m_known_attachments.end())
    return vk::AttachmentLoadOp::eNoneEXT;
  if (node->is_load())
    return vk::AttachmentLoadOp::eLoad;
  if (node->is_clear())
    return vk::AttachmentLoadOp::eClear;
  return vk::AttachmentLoadOp::eDontCare;
}

vk::AttachmentStoreOp RenderPass::get_store_op(Attachment const* attachment) const
{
  auto node = find_by_ID(m_known_attachments, attachment);
  if (node == m_known_attachments.end())
    return vk::AttachmentStoreOp::eNoneEXT;
  if (node->is_store() || node->is_preserve())
    return vk::AttachmentStoreOp::eStore;
  return vk::AttachmentStoreOp::eDontCare;
}

vk::AttachmentLoadOp RenderPass::get_stencil_load_op(Attachment const* attachment) const
{
  // Not implemented.
  ASSERT(false);
  return vk::AttachmentLoadOp::eDontCare;
}

vk::AttachmentStoreOp RenderPass::get_stencil_store_op(Attachment const* attachment) const
{
  // Not implemented.
  ASSERT(false);
  return vk::AttachmentStoreOp::eDontCare;
}

vk::ImageLayout RenderPass::get_optimal_layout(AttachmentNode const& node, bool separate_depth_stencil_layouts) const
{
  ImageViewKind const& image_view_kind = node.attachment()->image_view_kind();
  if (image_view_kind.is_color())
    return vk::ImageLayout::eColorAttachmentOptimal;
  else if (image_view_kind.is_depth_and_or_stencil())
  {
    if (image_view_kind.is_depth_stencil() || !separate_depth_stencil_layouts)
      return node.is_preserve() ? vk::ImageLayout::eDepthStencilReadOnlyOptimal
                                : vk::ImageLayout::eDepthStencilAttachmentOptimal;
    else if (image_view_kind.is_depth())
      return node.is_preserve() ? vk::ImageLayout::eDepthReadOnlyOptimal
                                : vk::ImageLayout::eDepthAttachmentOptimal;
    else if (image_view_kind.is_stencil())
      return node.is_preserve() ? vk::ImageLayout::eStencilReadOnlyOptimal
                                : vk::ImageLayout::eStencilAttachmentOptimal;
  }
  // Couldn't figure out optimal layout.
  ASSERT(false);
  return vk::ImageLayout::eGeneral;
}

vk::ImageLayout RenderPass::get_initial_layout(Attachment const* attachment, bool supports_separate_depth_stencil_layouts) const
{
  auto node = find_by_ID(m_known_attachments, attachment);
  if (node == m_known_attachments.end())
    return vk::ImageLayout::eUndefined;
  if (!node->is_source())
    return get_optimal_layout(*node, supports_separate_depth_stencil_layouts);
  vk::ImageLayout initial_layout = attachment->image_kind()->initial_layout;
  vk::ImageLayout final_layout = attachment->get_final_layout();
  if (final_layout != vk::ImageLayout::eUndefined && attachment->image_kind()->initial_layout != final_layout)
    THROW_ALERT("The initial_layout of the ImageKind of attachment \"[ATTACHMENT]\" should be [FINALLAYOUT], but it is [INITIALLAYOUT].",
        AIArgs("[ATTACHMENT]", attachment)("[FINALLAYOUT]", vk::to_string(final_layout))("[INITIALLAYOUT]", vk::to_string(initial_layout)));
  return initial_layout;
}

vk::ImageLayout RenderPass::get_final_layout(Attachment const* attachment, bool supports_separate_depth_stencil_layouts) const
{
  auto node = find_by_ID(m_known_attachments, attachment);
  if (node == m_known_attachments.end())
    return vk::ImageLayout::eUndefined;
  if (!node->is_sink() || !node->is_store())
    return get_optimal_layout(*node, supports_separate_depth_stencil_layouts);
  if (node->is_present())
    return vk::ImageLayout::ePresentSrcKHR;
  DoutFatal(dc::core, "Couldn't figure out the final layout of \"" << attachment->name() << "\".");
  ASSERT(false);
  return vk::ImageLayout::eGeneral;
}

void RenderPass::for_all_render_passes_until(
    int traversal_id, std::function<bool(RenderPass*, std::vector<RenderPass*>&)> const& lambda, SearchType search_type, std::vector<RenderPass*>& path, bool skip_lambda)
{
  DoutEntering(dc::rpverbose, "RenderPass::for_all_render_passes_until(" << traversal_id <<
      ", lambda, " << search_type << ", " << path << ", skip_lambda:" << std::boolalpha << skip_lambda << ") [" << this << "]");

  // Did we reach the end?
  if (m_traversal_id == traversal_id)
    return;
  m_traversal_id = traversal_id;

  if (!skip_lambda)
  {
    // Call the callback and also stop traversing the graph if it returns true.
    if (lambda(this, path))
      return;

    path.push_back(this);
  }

  switch (search_type)
  {
    case Subsequent:
      // Just folling the '>>' upstream.
      if (RenderPassStream* subsequent_render_pass = m_stream.subsequent_render_pass())
        subsequent_render_pass->owner()->for_all_render_passes_until(traversal_id, lambda, Subsequent, path);
      break;
    case Outgoing:
      // Traverse the graph upstream, depth first.
      for (RenderPass* node : m_outgoing_vertices)
        node->for_all_render_passes_until(traversal_id, lambda, Outgoing, path);
      break;
    case Incoming:
      // Traverse the graph downstream, depth first.
      for (RenderPass* node : m_incoming_vertices)
        node->for_all_render_passes_until(traversal_id, lambda, Incoming, path);
      break;
  }

  if (!skip_lambda)
    path.pop_back();
}

void RenderPass::add_attachments_to(std::set<Attachment const*, Attachment::CompareIDLessThan>& attachments)
{
  for (AttachmentNode const& node : m_known_attachments)
    attachments.insert(node.attachment());
}

void RenderPass::set_is_present_on_attachment_sink_with_index(AttachmentIndex index)
{
  for (AttachmentNode& node : m_known_attachments)
    if (node.is_sink() &&  node.render_graph_attachment_index() == index)
      node.set_is_present();
}

void RenderPass::stores_called(RenderPass* render_pass)
{
  auto res = m_stores_called.insert(render_pass);
  if (!res.second)
    THROW_ALERT("Render pass \"[RENDERPASS]\" occurs more than once in the graph", AIArgs("[RENDERPASS]", render_pass));
}

void RenderPass::create(task::SynchronousWindow const* owning_window)
{
  DoutEntering(dc::renderpass, "RenderPass:create(" << owning_window << ") [" << this << "]");
  // Create vk::AttachmentDescription objects.
  for (AttachmentNode const& node : m_known_attachments)
  {
    Attachment const* attachment = node.attachment();
    vk::Format const format = attachment->image_view_kind()->format;
    vk::SampleCountFlagBits const samples = attachment->image_kind()->samples;
    vk_defaults::AttachmentDescription attachment_description;
    attachment_description
      .setFormat(format)
      .setSamples(samples)
      .setLoadOp(get_load_op(attachment))
      .setStoreOp(get_store_op(attachment))
      ;
    if (attachment->image_view_kind().is_stencil())
    {
      attachment_description
        .setStencilLoadOp(get_stencil_load_op(attachment))
        .setStencilStoreOp(get_stencil_store_op(attachment))
        ;
    }
    bool supports_separate_depth_stencil_layouts = owning_window->logical_device()->supports_separate_depth_stencil_layouts();
    attachment_description.setInitialLayout(get_initial_layout(attachment, supports_separate_depth_stencil_layouts));
    attachment_description.setFinalLayout(get_final_layout(attachment, supports_separate_depth_stencil_layouts));

    Dout(dc::notice, "attachment_description " << node.render_pass_attachment_index() << " = " << attachment_description);
    m_attachment_descriptions.push_back(attachment_description);
  }

  // Create vk::SubpassDescription object (currently only one subpass is supported, which is sufficient on desktops).
  vk_defaults::SubpassDescription subpass_description;
  for (AttachmentNode const& node : m_known_attachments)
  {
    vk::AttachmentReference* attachment_reference_ptr = nullptr;
    Attachment const* attachment = node.attachment();
    if (attachment->image_view_kind().is_color())
      attachment_reference_ptr = &m_subpass_data.m_color_attachments.emplace_back();
    else if (attachment->image_view_kind().is_depth_and_or_stencil())
    {
      attachment_reference_ptr = &m_subpass_data.m_depth_stencil_attachment;
      // There should only be one depth/stencil attachment!
      ASSERT(attachment_reference_ptr->layout == vk::ImageLayout::eUndefined);
    }
    else
      THROW_ALERT("Don't know how to create a SubpassDescription for image view kind of [ATTACHMENT].", AIArgs("[ATTACHMENT]", attachment));
    attachment_reference_ptr->setAttachment(static_cast<uint32_t>(node.render_pass_attachment_index().get_value()))
      .setLayout(get_optimal_layout(node, false /* layout may not be DEPTH_ATTACHMENT_OPTIMAL|DEPTH_READ_ONLY_OPTIMAL|STENCIL_ATTACHMENT_OPTIMAL|STENCIL_READ_ONLY_OPTIMAL */));
  }
  // If we did not encounter a depth/stencil attachment then apparently we're not using it.
  if (m_subpass_data.m_depth_stencil_attachment.layout == vk::ImageLayout::eUndefined)
    m_subpass_data.m_depth_stencil_attachment.attachment = VK_ATTACHMENT_UNUSED;
  subpass_description
    .setColorAttachments(m_subpass_data.m_color_attachments)
    .setPDepthStencilAttachment(&m_subpass_data.m_depth_stencil_attachment);
  Dout(dc::notice, "subpass_description #0 = " << subpass_description);
  m_subpass_descriptions.push_back(subpass_description);

  // Finally really create the render pass.
  create_render_pass();
}

utils::Vector<vk::FramebufferAttachmentImageInfo, pAttachmentsIndex> RenderPass::get_framebuffer_attachment_image_infos(vk::Extent2D extent) const
{
  DoutEntering(dc::renderpass, "RenderPass::get_attachments_image_infos(" << extent << ")");
  utils::Vector<vk::FramebufferAttachmentImageInfo, pAttachmentsIndex> attachments_image_infos;
  for (AttachmentNode const& node : m_known_attachments)
  {
    ImageKind const& image_kind = node.attachment()->image_kind();
    attachments_image_infos.push_back({
        .usage = image_kind->usage,
        .width = extent.width,
        .height = extent.height,
        .layerCount = image_kind->array_layers,
        .viewFormatCount = 1,
        .pViewFormats = &image_kind->format
      });
  }
  return attachments_image_infos;
}

void RenderPass::print_on(std::ostream& os) const
{
  os << m_name;
}

#ifdef CWDEBUG
int get_id(RenderPass const* render_pass, std::map<RenderPass const*, int>& ids, std::vector<std::string>& names, int& next_id)
{
  auto res = ids.emplace(render_pass, next_id);
  if (res.second)
  {
    names.push_back(render_pass->name());
    ++next_id;
  }
  return res.first->second;
}

void RenderPass::print_vertices(std::map<RenderPass const*, int>& ids, std::vector<std::string>& names, int& next_id,
    std::vector<std::pair<int, int>>& forwards_edges, std::vector<std::pair<int, int>>& backwards_edges) const
{
  int id = get_id(this, ids, names, next_id);
  Dout(dc::renderpass|continued_cf, this << ": ");
  if (has_incoming_vertices())
  {
    Dout(dc::continued, "incoming: ");
    char const* prefix = "";
    for (RenderPass* from : m_incoming_vertices)
    {
      int from_id = get_id(from, ids, names, next_id);
      backwards_edges.emplace_back(id, from_id);
      Dout(dc::continued, prefix << from << '(' << from_id << ')');
      prefix = ", ";
    }
    if (has_outgoing_vertices())
      Dout(dc::continued, ", ");
  }
  if (has_outgoing_vertices())
  {
    Dout(dc::continued, "outgoing: ");
    char const* prefix = "";
    for (RenderPass* to : m_outgoing_vertices)
    {
      int to_id = get_id(to, ids, names, next_id);
      forwards_edges.emplace_back(id, to_id);
      Dout(dc::continued, prefix << to << '(' << to_id << ')');
      prefix = ", ";
    }
  }
  Dout(dc::finish, ".");
}
#endif

} // namespace vulkan::rendergraph
