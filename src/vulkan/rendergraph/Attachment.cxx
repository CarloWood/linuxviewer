#include "sys.h"
#include "debug/vulkan_print_on.h"
#include "Attachment.h"
#include "SynchronousWindow.h"
#include "debug/debug_ostream_operators.h"

namespace vulkan::rendergraph {

Attachment::Attachment(task::SynchronousWindow* owning_window, std::string const& name, ImageViewKind const& image_view_kind) :
  m_image_view_kind(image_view_kind),
  m_id(owning_window->attachment_id_context.get_id()),
  m_name(name)
{
  DoutEntering(dc::vulkan, "vulkan::rendergraph::Attachment::Attachment(" << owning_window << ", " << image_view_kind << ", \"" << name << "\"");
}

void Attachment::print_on(std::ostream& os) const
{
  os << m_name;
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
