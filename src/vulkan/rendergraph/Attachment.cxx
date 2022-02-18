#include "sys.h"
#include "Attachment.h"
#include "SynchronousWindow.h"
#include "debug/vulkan_print_on.h"
#include "debug/debug_ostream_operators.h"

namespace vulkan::rendergraph {

Attachment::Attachment(task::SynchronousWindow* owning_window, std::string const& name, ImageViewKind const& image_view_kind, bool is_swapchain_image) :
  m_owning_window(owning_window), m_image_view_kind(image_view_kind), m_name(name),
  m_clear_value(image_view_kind.is_depth_and_or_stencil() ? owning_window->render_graph().m_default_depth_stencil_clear_value : owning_window->render_graph().m_default_color_clear_value)
{
  DoutEntering(dc::vulkan, "vulkan::rendergraph::Attachment::Attachment(" << owning_window << ", " << image_view_kind << ", \"" << name << "\", image_view_kind, " << is_swapchain_image << ")");
  Dout(dc::vulkan, "Default clear value of attachment \"" << name << "\" is " << m_clear_value);
  // The swapchain attachment gets an `undefined` AttachmentIndex defined - so we can easily recognize it.
  if (is_swapchain_image)
    assign_swapchain_index();
}

void Attachment::print_on(std::ostream& os) const
{
  os << m_name;
}

void Attachment::assign_unique_index() const
{
  // Only call this function once, of course.
  ASSERT(undefined_index());
  m_render_graph_attachment_index.emplace(m_owning_window->attachment_index_context.get_id());
  m_owning_window->register_attachment(this);
}

#ifdef CWDEBUG
void Attachment::OpClear::print_on(std::ostream& os) const
{
  os << '~' << m_attachment->m_name;
}

void Attachment::OpRemoveOrDontCare::print_on(std::ostream& os) const
{
  os << '-' << m_attachment->m_name;
}

void Attachment::OpLoad::print_on(std::ostream& os) const
{
  os << '+' << m_attachment->m_name;
}
#endif

} // namespace vulkan::rendergraph
