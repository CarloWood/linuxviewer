#include "sys.h"
#include "Attachment.h"

namespace vulkan::rendergraph {

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
