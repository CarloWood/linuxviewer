#pragma once

#include "../ImageKind.h"
#include "utils/UniqueID.h"

namespace vulkan::rendergraph {

// Attachment.
//
// An Attachment is a unique object (within the specified context) with non-mutable data describing the attachment.
// The `context` passed to the constructor must be a member of the associated task::SynchronousWindow.
//
class Attachment
{
 private:
  ImageViewKind const& m_image_view_kind;       // Static description of the image view related to this attachment.
  utils::UniqueID<int> const m_id;              // Unique in the context of a given task::SynchronousWindow.
  std::string const m_name;                     // Human readable name of the attachment; e.g. "depth" or "output".
  mutable vk::ImageLayout m_final_layout = {};

 public:
  Attachment(utils::UniqueIDContext<int>& context, ImageViewKind const& image_view_kind, std::string const& name) :
    m_image_view_kind(image_view_kind), m_id(context.get_id()), m_name(name) { }

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

  ImageKind const& image_kind() const { return m_image_view_kind.image_kind(); }
  ImageViewKind const& image_view_kind() const { return m_image_view_kind; }

  utils::UniqueID<int> id() const { return m_id; }

  struct CompareIDLessThan
  {
    bool operator()(Attachment const* attachment1, Attachment const* attachment2) const
    {
      return attachment1->m_id < attachment2->m_id;
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
