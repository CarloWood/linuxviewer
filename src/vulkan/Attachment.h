#pragma once

#include "rendergraph/Attachment.h"
#include "TextureParameters.h"          // AttachmentIndex
#include "ClearValue.h"

namespace vulkan {

class Attachment : public rendergraph::Attachment
{
 private:
  ClearValue m_clear_value;             // Default color or depth/stencil clear value.
  AttachmentIndex m_index;              // Index/id that is unique within the render graph context of owning_window. For use with per-attachment arrays.
                                        // The index will be undefined() when this attachment is the color attachment of a swapchain image.

 private:
  Attachment(task::SynchronousWindow* owning_window, std::string const& name, ImageViewKind const& image_view_kind, bool is_swapchain_image);

 public:
  Attachment(task::SynchronousWindow* owning_window, std::string const& name, ImageViewKind const& image_view_kind) :
    Attachment(owning_window, name, image_view_kind, false) { }

  Attachment(utils::Badge<Swapchain>, task::SynchronousWindow* owning_window, std::string const& name, ImageViewKind const& image_view_kind) :
    Attachment(owning_window, name, image_view_kind, true) { }

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
    return m_index;
  }

  operator AttachmentIndex() const
  {
    return m_index;
  }
};

} // namespace vulkan
