#pragma once

#include "ImageKind.h"
#include "debug.h"
#include <utils/Vector.h>
#include <set>
#include <memory>

#if !defined(DOXYGEN)
NAMESPACE_DEBUG_CHANNELS_START
extern channel_ct renderpass;
NAMESPACE_DEBUG_CHANNELS_END
#endif

namespace vulkan {

struct Attachment;
struct RenderPass;
struct RenderPassStream;

using AttachmentIndex = utils::VectorIndex<Attachment>;

// An image view, being used as an attachment.
struct Attachment
{
  std::unique_ptr<ImageViewKind> m_kind_and_id;         // Use a unique_ptr so that this object automatically gets a unique ID; and can still be moved.
  std::string m_name;                                   // Human readable name of the attachment; e.g. "depth" or "output".

  Attachment(ImageViewKind const& image_view_kind, char const* name) : m_kind_and_id(std::make_unique<ImageViewKind>(image_view_kind)), m_name(name) { }
  Attachment(Attachment&&) = default;

  // The result type of ~attachment.
  struct OpClear
  {
    Attachment const* m_attachment;
    OpClear(Attachment const* attachment) : m_attachment(attachment) { }
  };

  // The result type of -attachment.
  struct OpRemove
  {
    Attachment const* m_attachment;
    OpRemove(Attachment const* attachment) : m_attachment(attachment) { }
  };

  // The result type of +attachment.
  struct OpAdd
  {
    Attachment const* m_attachment;
    OpAdd(Attachment const* attachment) : m_attachment(attachment) { }
  };

  OpClear operator~() const { return this; }
  OpRemove operator-() const { return this; }
  OpAdd operator+() const { return this; }

  ImageKind const& image_kind() const { return m_kind_and_id->image_kind(); }
  ImageViewKind const& image_view_kind() const { return *m_kind_and_id; }
};

// The attachment index of an Attachment in the context of a given RenderPass.
struct RenderPassAttachment
{
  AttachmentIndex m_index;                              // The attachment index in the render pass pAttachment array that this object belongs to.
  Attachment const* m_attachment;                       // The underlaying attachment.
  mutable bool m_clear = false;                         // Set to true if the attachment is prefixed with a tilde (~).
  mutable bool m_preserved = false;                     // Set if a later render pass loads this attachment, without that an intermediate render pass clears it or writes to it.

  RenderPassAttachment(AttachmentIndex index, Attachment const* attachment) : m_index(index), m_attachment(attachment) { }

  // Sort by attachment ID.
  struct CompareAttachment
  {
    bool operator()(RenderPassAttachment const& a1, RenderPassAttachment const& a2) const
    {
      return a1.m_attachment->m_kind_and_id.get() < a2.m_attachment->m_kind_and_id.get();
    }
  };

  // Sort by attachment index.
  struct CompareIndex
  {
    bool operator()(RenderPassAttachment const& a1, RenderPassAttachment const& a2) const
    {
      return a1.m_index < a2.m_index;
    }
  };
};

// The type used to form a render_pass chain with operator>>.
struct RenderPassStream
{
  RenderPass* m_render_pass;                            // The under laying render pass.
  RenderPassStream* m_prev_render_pass = nullptr;       // A double linked list of all render passes.
  RenderPassStream* m_next_render_pass = nullptr;

  RenderPassStream(RenderPass* render_pass) : m_render_pass(render_pass) { }

  template<typename... Args>
  RenderPassStream& write_to(Args const&... args);

  RenderPassStream& operator>>(RenderPassStream& out);
};

struct RenderPass
{
  RenderPassStream m_output;                            // A pointer to this object is returned by operator->.
  std::string m_name;                                   // Human readable name of this render pass.
  // These two are used by get_index.
  AttachmentIndex m_next_index{0};                      // The next attachment index to use for a new attachment.
  std::set<RenderPassAttachment, RenderPassAttachment::CompareAttachment> m_all_attachments;
  // These three store indices return by get_index.
  std::set<RenderPassAttachment, RenderPassAttachment::CompareIndex> m_barred_input_attachments;
  std::set<RenderPassAttachment, RenderPassAttachment::CompareIndex> m_added_input_attachments;
  std::set<RenderPassAttachment, RenderPassAttachment::CompareIndex> m_added_output_attachments;

  RenderPass(char const* name) : m_output(this), m_name(name) { }

  // Get RenderPassAttachment for the pair this RenderPass / Attachment a.
  RenderPassAttachment get_index(Attachment const* a);

  RenderPassAttachment const& add_input_attachment(Attachment const& a);
  RenderPassAttachment const& add_output_attachment(Attachment const& a);
  void add_output_attachment(Attachment::OpClear const& a);
  void bar_input_attachment(Attachment const& a);
  RenderPassStream* operator->();
  RenderPass& operator[](Attachment::OpRemove a);
  RenderPass& operator[](Attachment::OpAdd const& a);
  RenderPass& operator[](Attachment::OpClear const& a);

  template<typename CONTAINER>
  static /*typename CONTAINER::iterator*/ auto find_by_ID(CONTAINER& container, Attachment const& a);

  template<typename CONTAINER>
  static /*typename CONTAINER::iterator*/ auto find_by_ID(CONTAINER& container, RenderPassAttachment const& a);

  vk::AttachmentLoadOp get_load_op(Attachment const& a) const;
  vk::AttachmentStoreOp get_store_op(Attachment const& a) const;
  vk::ImageLayout get_initial_layout(Attachment const& a) const;
  vk::ImageLayout get_final_layout(Attachment const& a) const;
};

template<typename... Args>
RenderPassStream& RenderPassStream::write_to(Args const&... args)
{
  Dout(dc::renderpass, "write_to(" << join(", ", args...) << ")");
  (m_render_pass->add_output_attachment(args), ...);
  return *this;
}

struct RenderGraph
{
  std::list<RenderPass const*> m_graph;
  RenderPassStream const* m_render_pass_list = nullptr;         // The first render pass in this list.

  void operator=(RenderPassStream const& render_passes);
  void create();

#ifdef CWDEBUG
  static void testsuite();

  void has_with(char, Attachment const& a) const;
  void has_with(int in_out, Attachment const& a, int required_load_op, int required_store_op) const;
#endif
};

#if CW_DEBUG
std::ostream& operator<<(std::ostream& os, Attachment const& a);
std::ostream& operator<<(std::ostream& os, Attachment::OpClear const& a);
std::ostream& operator<<(std::ostream& os, Attachment::OpRemove const& a);
std::ostream& operator<<(std::ostream& os, Attachment::OpAdd const& a);
inline std::ostream& operator<<(std::ostream& os, Attachment const* ap) { return os << *ap; }
inline std::ostream& operator<<(std::ostream& os, RenderPassAttachment const& a) { return os << *a.m_attachment; }
inline std::ostream& operator<<(std::ostream& os, RenderPass const* render_pass) { return os << render_pass->m_name; }
#endif // CW_DEBUG

//static
template<typename CONTAINER>
/*typename CONTAINER::iterator*/ auto RenderPass::find_by_ID(CONTAINER& container, Attachment const& a)
{
  struct CompareEqualID
  {
    Attachment const& m_attachment;
    CompareEqualID(Attachment const& attachment) : m_attachment(attachment) { }
    bool operator()(RenderPassAttachment const& attachment) { return m_attachment.m_kind_and_id.get() == attachment.m_attachment->m_kind_and_id.get(); }
  };

  return std::find_if(container.begin(), container.end(), CompareEqualID{a});
}

//static
template<typename CONTAINER>
/*typename CONTAINER::iterator*/ auto RenderPass::find_by_ID(CONTAINER& container, RenderPassAttachment const& a)
{
  struct CompareEqualID
  {
    RenderPassAttachment const& m_attachment;
    CompareEqualID(RenderPassAttachment const& attachment) : m_attachment(attachment) { }
    bool operator()(RenderPassAttachment const& attachment) { return m_attachment.m_attachment->m_kind_and_id.get() == attachment.m_attachment->m_kind_and_id.get(); }
  };

  return std::find_if(container.begin(), container.end(), CompareEqualID{a});
}

} // namespace vulkan
