#include "sys.h"
#include "Attachment.h"

namespace vulkan::rendergraph {

#ifdef CWDEBUG
void Attachment::print_on(std::ostream& os) const
{
  os << m_name;
}

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
