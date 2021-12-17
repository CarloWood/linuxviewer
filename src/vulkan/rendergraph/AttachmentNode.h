#pragma once

#include "Attachment.h"
#include "LoadStoreOps.h"
#include "utils/Vector.h"
#include "utils/AIAlert.h"

namespace vulkan::rendergraph {

class RenderPass;
class AttachmentNode;

// AttachmentIndex.
//
// A wrapper around a size_t that can be used into a vector of attachments known by given render pass.
//
using AttachmentIndex = utils::VectorIndex<AttachmentNode>;

// AttachmentNode.
//
// A node of of the subgraph for a specific a attachment.
//
class AttachmentNode
{
 private:
  RenderPass const* m_render_pass;                                      // The render pass this node falls under.
  Attachment const* m_attachment;                                       // A pointer to the associated Attachment description.
  AttachmentIndex m_index;                                              // The render pass specific index (index into pAttachments).
  LoadStoreOps m_ops;                                                   // Encodes the [DCL][DS] code for this attachment/render_pass.
  // The attachment graph.
  std::vector<AttachmentNode*> m_preceding_nodes;
  std::vector<AttachmentNode*> m_subsequent_nodes;

 public:
  AttachmentNode(RenderPass const* render_pass, Attachment const* attachment, AttachmentIndex index) :
    m_render_pass(render_pass), m_attachment(attachment), m_index(index) { }

  utils::UniqueID<int> id() const { return m_attachment->id(); }

  void set_load()
  {
    try
    {
      m_ops.set_load();
    }
    catch (AIAlert::Error const& error)
    {
      THROW_ALERT("Attachment \"[ATTACHMENT]\", render pass \"[RENDERPASS]\"", AIArgs("[ATTACHMENT]", m_attachment)("[RENDERPASS]", m_render_pass), error);
    }
  }
  void set_clear()
  {
    try
    {
      m_ops.set_clear();
    }
    catch (AIAlert::Error const& error)
    {
      THROW_ALERT("Attachment \"[ATTACHMENT]\", render pass \"[RENDERPASS]\"", AIArgs("[ATTACHMENT]", m_attachment)("[RENDERPASS]", m_render_pass), error);
    }
  }
  void set_store() { m_ops.set_store(); }
  bool is_load() const { return m_ops.is_load(); }
  bool is_clear() const { return m_ops.is_clear(); }
  bool is_store() const { return m_ops.is_store(); }

  // Automatic conversion to just the Attachment.
  operator Attachment const*() const { return m_attachment; }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::rendergraph
