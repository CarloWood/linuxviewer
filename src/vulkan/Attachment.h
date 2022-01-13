#pragma once

#include "rendergraph/Attachment.h"
#include "ImageParameters.h"
#include "ClearValue.h"

namespace vulkan {

class Attachment : public rendergraph::Attachment
{
 private:
  ClearValue m_clear_value;             // Default color or depth/stencil clear value.
  AttachmentIndex m_index;              // Index/id that is unique within the render graph context of owning_window. For use with per-attachment arrays.

 public:
  Attachment(task::SynchronousWindow* owning_window, std::string const& name, ImageViewKind const& image_view_kind);

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
