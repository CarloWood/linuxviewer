#include "sys.h"
#include "RenderPassStream.h"
#include "AttachmentNode.h"
#include "RenderPass.h"

namespace vulkan::rendergraph {

RenderPassStream& RenderPassStream::operator>>(RenderPassStream& subsequent_render_pass)
{
  Dout(dc::renderpass, *this << " >> " << subsequent_render_pass);

  // For each known attachment that is stored, inform subsequent_render_pass.
  for (AttachmentNode const& node : m_owner->m_known_attachments)
    if (node.is_store())
      subsequent_render_pass.m_owner->preceding_render_pass_stores(node);

  // Finalize [-attachment] modifiers.
  subsequent_render_pass.do_load_dont_cares();

  link(subsequent_render_pass);
  return subsequent_render_pass;
}

void RenderPassStream::link(RenderPassStream& subsequent_render_pass)
{
  // Construct double linked list of render passes that are connected with operator>>'s.
  m_subsequent_render_pass = &subsequent_render_pass;
  subsequent_render_pass.m_preceding_render_pass = this;
}

void RenderPassStream::stores_called(RenderPass* render_pass)
{
  if (m_preceding_render_pass)
    m_preceding_render_pass->stores_called(render_pass);
  m_owner->stores_called(render_pass);
}

#ifdef CWDEBUG
void RenderPassStream::print_on(std::ostream& os) const
{
  os << m_owner->m_name;
}
#endif

} // namespace vulkan::rendergraph
