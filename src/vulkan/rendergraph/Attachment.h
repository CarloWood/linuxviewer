#pragma once

#include "../ImageKind.h"
#include "ClearValue.h"
#include "utils/UniqueID.h"
#include "utils/Vector.h"

namespace vulkan::task {
class SynchronousWindow;
} // namespace vulkan::task

namespace vulkan::rendergraph {

class Attachment;
using AttachmentIndex = utils::VectorIndex<Attachment const*>;

// Attachment.
//
// An Attachment is a unique object (within the specified context) with non-mutable data describing the attachment.
// The `context` passed to the constructor must be a member of the associated task::SynchronousWindow.
//
class Attachment
{
 private:
  task::SynchronousWindow* m_owning_window;
  ImageViewKind const& m_image_view_kind;       // Static description of the image view related to this attachment.
  ClearValue m_clear_value;                     // Default color or depth/stencil clear value.

  // Index into vectors with all attachments known to the render graph (excluding the swapchain attachment).
  //
  // Mutable because it is late-assigned, just in time for use, by assign_unique_index() const.
  // The function assign_unique_index is const because it is called from a point where we really only have const access.
  // Since m_render_graph_attachment_index is never read before it is assigned, it is ok to pretend that it was already assigned and thus
  // that assign_unique_index() isn't really doing anything and thus can be const.
  // The std::optional is used to keep track of whether or not the index was already assigned a unique value or not.
  mutable std::optional<utils::UniqueID<AttachmentIndex>> m_render_graph_attachment_index;

  std::string const m_name;                             // Human readable name of the attachment; e.g. "depth" or "output".
  mutable vk::ImageLayout m_final_layout = {};

 private:
  Attachment(task::SynchronousWindow* owning_window, std::string const& name, ImageViewKind const& image_view_kind, bool is_swapchain_image);

 public:
  Attachment(task::SynchronousWindow* owning_window, std::string const& name, ImageViewKind const& image_view_kind) :
    Attachment(owning_window, name, image_view_kind, false) { }

  Attachment(utils::Badge<Swapchain>, task::SynchronousWindow* owning_window, std::string const& name, ImageViewKind const& image_view_kind) :
    Attachment(owning_window, name, image_view_kind, true) { }

  Attachment(Attachment const&) = delete;

  // The result type of ~attachment.
  struct OpClear
  {
    Attachment const* m_attachment;
    OpClear(Attachment const* attachment) : m_attachment(attachment) { }

#ifdef CWDEBUG
    void print_on(std::ostream& os) const;
#endif
  };

  // The result type of +attachment.
  struct OpLoad
  {
    Attachment const* m_attachment;
    OpLoad(Attachment const* attachment) : m_attachment(attachment) { }

#ifdef CWDEBUG
    void print_on(std::ostream& os) const;
#endif
  };

  // The result type of -attachment.
  struct OpRemoveOrDontCare
  {
    Attachment const* m_attachment;
    OpRemoveOrDontCare(Attachment const* attachment) : m_attachment(attachment) { }

#ifdef CWDEBUG
    void print_on(std::ostream& os) const;
#endif
  };

  OpClear operator~() const { return this; }
  OpLoad operator+() const { return this; }
  OpRemoveOrDontCare operator-() const { return this; }

  std::string const& name() const { return m_name; }
  ImageKind const& image_kind() const { return m_image_view_kind.image_kind(); }
  ImageViewKind const& image_view_kind() const { return m_image_view_kind; }

  utils::UniqueID<AttachmentIndex> render_graph_attachment_index() const
  {
    ASSERT(m_render_graph_attachment_index.has_value());
    return m_render_graph_attachment_index.value();
  }
  bool undefined_index() const { return !m_render_graph_attachment_index.has_value(); }
  void assign_swapchain_index() const { m_render_graph_attachment_index.emplace(utils::UniqueIDContext<AttachmentIndex>{{}}.get_id()); }
  void assign_unique_index() const;

  struct CompareIDLessThan
  {
    bool operator()(Attachment const* attachment1, Attachment const* attachment2) const
    {
      return attachment1->render_graph_attachment_index() < attachment2->render_graph_attachment_index();
    }
  };

  // Called by rendergraph::RenderGraph::create.
  void set_final_layout(vk::ImageLayout layout) const
  {
    // This function should only be called once per attachment because only one sink per attachment is allowed (this is THE final layout).
    ASSERT(m_final_layout == vk::ImageLayout::eUndefined);
    m_final_layout = layout;
  }
  vk::ImageLayout get_final_layout() const
  {
    return m_final_layout;
  }

  // These used to be part of vulkan::Attachment, when that was still derived from this class.
  // Its more of a usage interface - not a rendergraph generation interface.

  void set_clear_value(ClearValue clear_value)
  {
    m_clear_value = clear_value;
  }

  ClearValue const& get_clear_value() const
  {
    return m_clear_value;
  }

  AttachmentIndex index() const
  {
    return render_graph_attachment_index();
  }

  operator AttachmentIndex() const
  {
    return render_graph_attachment_index();
  }

  void print_on(std::ostream& os) const;
  friend std::ostream& operator<<(std::ostream& os, Attachment const& attachment) { attachment.print_on(os); return os; }
};

// Allow printing an Attachment* as if it is an Attachment.
inline std::ostream& operator<<(std::ostream& os, Attachment const* attachment) { attachment->print_on(os); return os; }
#ifdef CWDEBUG
// These print_on's aren't picked up by ADL.
inline std::ostream& operator<<(std::ostream& os, Attachment::OpClear const& mod_attachment) { mod_attachment.print_on(os); return os; }
inline std::ostream& operator<<(std::ostream& os, Attachment::OpLoad const& mod_attachment) { mod_attachment.print_on(os); return os; }
inline std::ostream& operator<<(std::ostream& os, Attachment::OpRemoveOrDontCare const& mod_attachment) { mod_attachment.print_on(os); return os; }
#endif

} // namespace vulkan::rendergraph
