#include "sys.h"
#include "Attachment.h"
#include "SynchronousWindow.h"
#include "debug/vulkan_print_on.h"
#include "debug/debug_ostream_operators.h"

namespace vulkan::rendergraph {

Attachment::Attachment(task::SynchronousWindow* owning_window, std::string const& name, ImageViewKind const& image_view_kind, bool is_swapchain_image) :
  m_owning_window(owning_window), m_image_view_kind(image_view_kind), m_name(name)
{
  DoutEntering(dc::vulkan, "vulkan::rendergraph::Attachment::Attachment(" << owning_window << ", " << image_view_kind << ", \"" << name << "\"");
}

void Attachment::print_on(std::ostream& os) const
{
  os << m_name;
}

void Attachment::assign_unique_index() const
{
  // Only call this function once, of course.
  ASSERT(undefined_index());
  m_rendergraph_attachment_index.emplace(m_owning_window->attachment_index_context.get_id());
  m_owning_window->register_attachment(static_cast<vulkan::Attachment const*>(this));
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
