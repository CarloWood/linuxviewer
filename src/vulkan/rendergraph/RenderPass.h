// Must be included first.
#include "RenderPassStream.h"

#ifndef RENDER_PASS_H
#define RENDER_PASS_H

#include "AttachmentNode.h"
#include "Attachment.h"
#include <string>
#include "debug.h"

#if !defined(DOXYGEN)
NAMESPACE_DEBUG_CHANNELS_START
extern channel_ct renderpass;
NAMESPACE_DEBUG_CHANNELS_END
#endif

namespace vulkan::rendergraph {

// RenderPass.
//
// A RenderPass is a unique object (within the construction of a given RenderGraph).
//
class RenderPass
{
 private:
  std::string m_name;                                                   // Human readable name of this render pass.
  utils::Vector<AttachmentNode> m_known_attachments;                    // Vector with all known attachments of this render pass.
  AttachmentIndex m_next_index{0};                                      // The next attachment index to use for a new attachment node.
  std::vector<Attachment const*> m_remove_or_dontcare_attachments;      // Temporary storage for attachments listed with `[-attachment]`.
  RenderPassStream m_stream;                                            // Helper object that operator-> points to.

  friend class RenderPassStream;                                        // The m_stream object is allowed to access all members of its owner.

 public:
  // Constructor.
  RenderPass(char const* name) : m_name(name), m_stream(this) { }

  // Modifiers.
  RenderPass& operator[](Attachment::OpRemoveOrDontCare mod_attachment);
  RenderPass& operator[](Attachment::OpLoad mod_attachment);
  RenderPass& operator[](Attachment::OpClear mod_attachment);
  void do_load_dont_cares();

  // Add attachment to the attachments that are stored.
  void store_attachment(Attachment const& attachment);
  void store_attachment(Attachment::OpClear mod_attachment);

  // Accessor of m_stream.
  RenderPassStream* operator->() { return &m_stream; }

  // Results.
  vk::AttachmentLoadOp get_load_op(Attachment const& attachment) const;
  vk::AttachmentStoreOp get_store_op(Attachment const& attachment) const;

  //---------------------------------------------------------------------------
  // Utilities

  // Get a (new) RenderPassNode for the pair this RenderPass / some Attachment.
  // Calling this makes the attachment known (if it wasn't before). It does
  // not connect the node in the attachment subgraph.
  AttachmentNode& get_node(Attachment const* attachment);

  bool is_known(Attachment const& attachment) const;

  // Search for Attachment by ID in a container with AttachmentNode's.
  template<typename AttachmentNodes>
  static auto find_by_ID(AttachmentNodes& container, Attachment const* attachment);

  // Search for AttachmentNode by ID in a container with AttachmentNode's.
  template<typename AttachmentNodes>
  static auto find_by_ID(AttachmentNodes& container, AttachmentNode const& node);

 private:
  void preceding_render_pass_stores(Attachment const* attachment);

#ifdef CWDEBUG
 public:
  void print_on(std::ostream& os) const;
  // Used by testsuite.
  bool remove_or_dontcare_attachments_empty() const { return m_remove_or_dontcare_attachments.empty(); }
#endif
};

//static
template<typename AttachmentNodes>
auto RenderPass::find_by_ID(AttachmentNodes& container, Attachment const* attachment)
{
  struct CompareEqualID
  {
    Attachment const* m_attachment;
    CompareEqualID(Attachment const* attachment) : m_attachment(attachment) { }
    bool operator()(AttachmentNode const& node) { return m_attachment->id() == node.id(); }
    bool operator()(Attachment const* attachment) { return m_attachment->id() == attachment->id(); }
  };

  return std::find_if(container.begin(), container.end(), CompareEqualID{attachment});
}

//static
template<typename AttachmentNodes>
auto RenderPass::find_by_ID(AttachmentNodes& container, AttachmentNode const& node)
{
  struct CompareEqualID
  {
    AttachmentNode const& m_node;
    CompareEqualID(AttachmentNode const& node) : m_node(node) { }
    bool operator()(AttachmentNode const& node) { return m_node.id() == node.id(); }
  };

  return std::find_if(container.begin(), container.end(), CompareEqualID{node});
}

#ifdef CWDEBUG
// Allow printing a RenderPass* as if it is a RenderPass.
inline std::ostream& operator<<(std::ostream& os, RenderPass const* render_pass) { render_pass->print_on(os); return os; }
#endif

} // namespace vulkan::rendergraph

#endif // RENDER_PASS_H
