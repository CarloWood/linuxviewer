#include "sys.h"
#include "Attachment.h"
#include "SynchronousWindow.h"

namespace vulkan {

Attachment::Attachment(task::SynchronousWindow* owning_window, std::string const& name, ImageViewKind const& image_view_kind, bool is_swapchain_image) :
  rendergraph::Attachment(owning_window, name, image_view_kind, is_swapchain_image),
  m_clear_value(owning_window->get_default_clear_value(image_view_kind.is_depth_and_or_stencil()))
{
  DoutEntering(dc::vulkan, "vulkan::Attachment::Attachment(" << owning_window << ", " << image_view_kind << ", \"" << name << "\", image_view_kind, " << is_swapchain_image << ")");
  Dout(dc::vulkan, "Default clear value of attachment \"" << name << "\" is " << m_clear_value);
  // The swapchain attachment gets an `undefined` AttachmentIndex defined - so we can easily recognize it.
  if (is_swapchain_image)
    assign_swapchain_index();
}

} // namespace vulkan
