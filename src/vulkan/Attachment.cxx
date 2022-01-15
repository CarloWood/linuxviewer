#include "sys.h"
#include "Attachment.h"
#include "SynchronousWindow.h"

namespace vulkan {

Attachment::Attachment(task::SynchronousWindow* owning_window, std::string const& name, ImageViewKind const& image_view_kind, bool is_swapchain_image) :
  rendergraph::Attachment(owning_window, name, image_view_kind),
  m_clear_value(owning_window->get_default_clear_value(image_view_kind.is_depth_and_or_stencil())),
  m_index(is_swapchain_image ? utils::UniqueIDContext<vulkan::AttachmentIndex>{{}}.get_id() : owning_window->render_graph_context().get_id())
{
  DoutEntering(dc::vulkan, "vulkan::Attachment::Attachment(" << owning_window << ", " << image_view_kind << ", \"" << name << "\"");
  Dout(dc::vulkan, "Default clear value of attachment \"" << name << "\" is " << m_clear_value);
  owning_window->register_attachment(this);
}

} // namespace vulkan
