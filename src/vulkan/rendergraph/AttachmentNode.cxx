#include "sys.h"
#include "AttachmentNode.h"
#include "debug.h"

namespace vulkan::rendergraph {

#ifdef CWDEBUG
void AttachmentNode::print_on(std::ostream& os) const
{
  os << m_render_pass << '/' << m_attachment << ' ' << m_index;
}
#endif

} // namespace vulkan::rendergraph
