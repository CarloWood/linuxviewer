// Must be included first.
#include "RenderPassStream.h"

#ifndef RENDER_PASS_H
#define RENDER_PASS_H

#include "AttachmentNode.h"
#include "Attachment.h"
#include <string>
#include <functional>
#include <set>
#include "debug.h"
#ifdef CWDEBUG
#include <map>
#include <vector>
#endif

#if defined(CWDEBUG) && !defined(DOXYGEN)
NAMESPACE_DEBUG_CHANNELS_START
extern channel_ct renderpass;
extern channel_ct rpverbose;
NAMESPACE_DEBUG_CHANNELS_END
#endif

namespace task {
class SynchronousWindow;
} // namespace task

namespace vulkan::rendergraph {

class RenderGraph;

// RenderPass.
//
// A RenderPass is a unique object (within the construction of a given RenderGraph).
//
class RenderPass
{
  friend class RenderPassStream;                                        // The m_stream object is allowed to access all members of its owner.

 private:
  std::string m_name;                                                   // Human readable name of this render pass.
  utils::Vector<AttachmentNode> m_known_attachments;                    // Vector with all known attachments of this render pass.
  AttachmentIndex m_next_index{0};                                      // The next attachment index to use for a new attachment node.
  std::vector<Attachment const*> m_remove_or_dontcare_attachments;      // Temporary storage for attachments listed with `[-attachment]`.
  RenderPassStream m_stream;                                            // Helper object that operator-> points to.
  std::set<RenderPass*> m_stores_called;
  // Graph generation.
  int m_traversal_id = {};                                              // Unique ID to identify which RenderPass nodes have already visited.
  std::set<RenderPass*> m_incoming_vertices;
  std::set<RenderPass*> m_outgoing_vertices;

  // RenderPass::create:
  utils::Vector<vk_defaults::AttachmentDescription, AttachmentIndex> m_attachment_descriptions;
                                                                        // Attachment descriptions corresponding to the attachment nodes of m_known_attachments.

 public:
  // Constructor (only called by RenderGraph::create_render_pass.
  // Use:
  //   auto& my_pass = my_render_graph.create_render_pass("my_pass"); to get a reference to a RenderPass.
  RenderPass(utils::Badge<RenderGraph>, std::string const& name) : m_name(name), m_stream(this) { }
  RenderPass(RenderPass const&&) = delete;      // Not allowed because m_stream contains a pointer back to this object.

 public:
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

  // Graph generation.
  void add_incoming_vertex(RenderPass* node) { m_incoming_vertices.insert(node); }
  void add_outgoing_vertex(RenderPass* node) { m_outgoing_vertices.insert(node); }
  utils::Vector<AttachmentNode> const& known_attachments() const { return m_known_attachments; }

  // Allow using raw RenderPass objects to add render graph vertices between render passes.
  friend RenderPassStream& operator>>(RenderPassStream& stream, RenderPass& render_pass) { stream.link(render_pass.m_stream); return render_pass.m_stream; }
  friend RenderPassStream& operator>>(RenderPass& render_pass, RenderPassStream& stream) { render_pass.m_stream.link(stream); return stream; }
  friend RenderPassStream& operator>>(RenderPass& render_pass1, RenderPass& render_pass2) { render_pass1.m_stream.link(render_pass2.m_stream); return render_pass2.m_stream; }

  // Results.
  vk::AttachmentLoadOp get_load_op(Attachment const* attachment) const;
  vk::AttachmentStoreOp get_store_op(Attachment const* attachment) const;
  vk::AttachmentLoadOp get_stencil_load_op(Attachment const* attachment) const;
  vk::AttachmentStoreOp get_stencil_store_op(Attachment const* attachment) const;
  vk::ImageLayout get_initial_layout(Attachment const* attachment) const;
  vk::ImageLayout get_final_layout(Attachment const* attachment) const;

  // Actual creation.
  void create(task::SynchronousWindow const* owning_window);

  // Harvest information.
  utils::Vector<vk_defaults::AttachmentDescription, AttachmentIndex> const& attachment_descriptions() const { return m_attachment_descriptions; }
  utils::Vector<vk::FramebufferAttachmentImageInfo, AttachmentIndex> get_framebuffer_attachment_image_infos(vk::Extent2D extent) const;

  //---------------------------------------------------------------------------
  // Utilities

  // Get a (new) RenderPassNode for the pair this RenderPass / some Attachment.
  // Calling this makes the attachment known (if it wasn't before). It does
  // not connect the node in the attachment subgraph.
  AttachmentNode& get_node(Attachment const* attachment);

  void stores_called(RenderPass* render_pass);

  bool is_known(Attachment const* attachment) const;
  bool is_load(Attachment const* attachment) const;
  bool is_clear(Attachment const* attachment) const;
  bool is_store(Attachment const* attachment) const;
  bool has_incoming_vertices() const { return !m_incoming_vertices.empty(); }
  bool has_outgoing_vertices() const { return !m_outgoing_vertices.empty(); }

  // Search for Attachment by ID in a container with AttachmentNode's.
  template<typename AttachmentNodes>
  static auto find_by_ID(AttachmentNodes& container, Attachment const* attachment);

  // Search for AttachmentNode by ID in a container with AttachmentNode's.
  template<typename AttachmentNodes>
  static auto find_by_ID(AttachmentNodes& container, AttachmentNode const& node);

  enum SearchType { Subsequent, Outgoing, Incoming };
  void for_all_render_passes_until(int traversal_id, std::function<bool(RenderPass*, std::vector<RenderPass*>&)> const& lambda,
      SearchType search_type, std::vector<RenderPass*>& path, bool skip_lambda = false);
  void add_attachments_to(std::set<Attachment const*, Attachment::CompareIDLessThan>& attachments);
  void set_is_present_on_attachment_sink_with_id(utils::UniqueID<int> id);

 private:
  void preceding_render_pass_stores(Attachment const* attachment);
  vk::ImageLayout get_optimal_layout(AttachmentNode const& node) const;

 public:
  void print_on(std::ostream& os) const;

#ifdef CWDEBUG
 public:
  // Used by testsuite.
  bool remove_or_dontcare_attachments_empty() const { return m_remove_or_dontcare_attachments.empty(); }

  std::string const& name() const { return m_name; }
  void print_vertices(std::map<RenderPass const*, int>& ids, std::vector<std::string>& names, int& next_id,
      std::vector<std::pair<int, int>>& forwards_edges, std::vector<std::pair<int, int>>& backwards_edges) const;

  static std::string to_string(SearchType search_type);
  friend std::ostream& operator<<(std::ostream& os, RenderPass::SearchType search_type) { return os << to_string(search_type); }
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

// Allow printing a RenderPass* as if it is a RenderPass.
inline std::ostream& operator<<(std::ostream& os, RenderPass const* render_pass) { render_pass->print_on(os); return os; }

} // namespace vulkan::rendergraph

#endif // RENDER_PASS_H
