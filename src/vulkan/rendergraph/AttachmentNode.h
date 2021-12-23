#pragma once

#include "Attachment.h"
#include "LoadStoreOps.h"
#include "utils/Vector.h"
#include "utils/AIAlert.h"
#include "debug.h"

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
  bool m_is_readonly = false;
  bool m_is_source = false;
  bool m_is_sink = false;
  bool m_is_present = false;

 public:
  AttachmentNode(RenderPass const* render_pass, Attachment const* attachment, AttachmentIndex index) :
    m_render_pass(render_pass), m_attachment(attachment), m_index(index) { }

  utils::UniqueID<int> id() const { return m_attachment->id(); }
  Attachment const* attachment() const { return m_attachment; }

  void set_load();
  void set_clear();
  void set_store();
  void set_preserve();

  void set_is_readonly();
  void set_is_source();
  void set_is_sink();
  void set_is_present();

  bool is_load() const { return m_ops.is_load(); }
  bool is_clear() const { return m_ops.is_clear(); }
  bool is_store() const { return m_ops.is_store(); }
  bool is_preserve() const { return m_ops.is_preserve(); }
  bool is_readonly() const { return m_is_readonly; }
  bool is_source() const { return m_is_source; }
  bool is_sink() const { return m_is_sink; }
  bool is_present() const { return m_is_present; }

  // Automatic conversion to just the Attachment.
  operator Attachment const*() const { return m_attachment; }

  // Accessor.
  RenderPass const* render_pass() const { return m_render_pass; }
  AttachmentIndex index() const { return m_index; }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::rendergraph
