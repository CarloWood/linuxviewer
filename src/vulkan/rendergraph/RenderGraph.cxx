#include "sys.h"
#include "RenderGraph.h"
#include "RenderPass.h"
#include "Attachment.h"
#include "debug.h"
#ifdef CWDEBUG
#include "utils/AIAlert.h"
#include "utils/debug_ostream_operators.h"
#endif

namespace vulkan::rendergraph {

void RenderGraph::create()
{
  // Fix AttachmentNode::m_preceding_nodes and AttachmentNode::m_subsequent_nodes

  // As every render pass is reachable from one or more of the sources,
  // we can run over all sources, and from there follow the

}

void RenderGraph::operator=(RenderPassStream& sink)
{
  RenderPassStream& source = sink.get_source();
  source.do_load_dont_cares();
  m_sinks.push_back(sink.owner());
  m_sources.push_back(source.owner());
}

#ifdef CWDEBUG

#define DECLARATION \
    [[maybe_unused]] bool illegal = false; \
    [[maybe_unused]] RenderPass lighting("lighting"); \
    [[maybe_unused]] RenderPass shadow("shadow"); \
    RenderPass render_pass("render_pass"); \
    RenderGraph render_graph; \
    Dout(dc::renderpass, "====================== Next Test ===========================")

#define TEST(test) \
    DECLARATION; \
    Dout(dc::renderpass, "Running test: " << #test); \
    render_graph = test; \
    render_graph.create();

//static
void RenderGraph::testsuite()
{
  constexpr char none = 0;      // char for overloading purposes.
  constexpr int in = 1;
  constexpr int out = 2;
  constexpr int internal = 4;

  constexpr int DONT_CARE = 0;
  constexpr int LOAD = 1;
  constexpr int CLEAR = 2;
  constexpr int STORE = 3;

  vulkan::ImageKind const k1{{}};
  vulkan::ImageViewKind const v1{k1, {}};
  utils::UniqueIDContext<int> context;
  Attachment const specular{context, v1, "specular"};
  Attachment const output{context, v1, "output"};

  {
    TEST(render_pass->stores(output));                        // attachment not used.
    render_graph.has_with(none, specular);
  }
  {
    // Special notation for { DONT_CARE, DONT_CARE }.
    TEST(render_pass[-specular]->stores(output));             // { DONT_CARE, DONT_CARE }
    render_graph.has_with(internal, specular, DONT_CARE, DONT_CARE);
  }
  // Equivalent to:
  {
    TEST(lighting->stores(output) >> render_pass[-specular]->stores(output));       // { DONT_CARE, DONT_CARE }
    render_graph.has_with(internal, specular, DONT_CARE, DONT_CARE);
  }
  {
    TEST(render_pass[+specular]->stores(output));             // { LOAD, DONT_CARE }
    render_graph.has_with(in, specular, LOAD, DONT_CARE);
  }
  {
    TEST(render_pass[~specular]->stores(output));             // { CLEAR, DONT_CARE }
    render_graph.has_with(internal, specular, CLEAR, DONT_CARE);
  }
  {
    TEST(render_pass->stores(specular));                      // { DONT_CARE, STORE }
    render_graph.has_with(out, specular, DONT_CARE, STORE);
  }
  {
    bool illegal = false;
    try {
      TEST(render_pass[-specular]->stores(specular));         // AIAlert: Output attachment "specular" is barred. illegal; Can't bar an attachment that wasn't just written.
    } catch (AIAlert::Error const& error) {
      Dout(dc::renderpass, "Illegal: " << error);
      illegal = true;
    }
    ASSERT(illegal);
  }
  {
    TEST(render_pass[+specular]->stores(specular));           // { LOAD, STORE }
    render_graph.has_with(in|out, specular, LOAD, STORE);
  }
  {
    bool illegal = false;
    try {
      TEST(render_pass[~specular]->stores(specular));         // { CLEAR, STORE } or shorter:
    } catch (AIAlert::Error const& error) {
      Dout(dc::renderpass, "Illegal: " << error);
      illegal = true;
    }
    ASSERT(illegal);
  }
  {
    TEST(render_pass->stores(~specular));                     // { CLEAR, STORE }
    render_graph.has_with(out, specular, CLEAR, STORE);
  }
  {
    bool illegal = false;
    try {
      TEST(render_pass[-specular]->stores(~specular));        // AIAlert: Output attachment "specular" is barred. { CLEAR, STORE }  warning?
    } catch (AIAlert::Error const& error) {
      Dout(dc::renderpass, "Illegal: " << error);
      illegal = true;
    }
    ASSERT(illegal);
  }
  {
    bool illegal = false;
    try {
      TEST(render_pass[+specular]->stores(~specular));        // illegal (can't have + and ~ at the same time).
    } catch (AIAlert::Error const& error) {
      Dout(dc::renderpass, "Illegal: " << error);
      illegal = true;
    }
    ASSERT(illegal);
  }
  {
    bool illegal = false;
    try {
      TEST(render_pass[~specular]->stores(~specular));        // { CLEAR, STORE }  warning?
    } catch (AIAlert::Error const& error) {
      Dout(dc::renderpass, "Illegal: " << error);
      illegal = true;
    }
    ASSERT(illegal);
  }
  {
    TEST(lighting->stores(~specular) >> render_pass->stores(output));               // { LOAD, DONT_CARE }
    render_graph.has_with(in, specular, LOAD, DONT_CARE);
  }
  {
    TEST(lighting->stores(~specular) >> render_pass[-specular]->stores(output));    // attachment not used.
    render_graph.has_with(none, specular);
  }
  {
    bool illegal = false;
    try {
      TEST(lighting->stores(~specular) >> render_pass[+specular]->stores(output));  // { LOAD, DONT_CARE } - but over specified. Lets not accept this.
    } catch (AIAlert::Error const& error) {
      Dout(dc::renderpass, "Illegal: " << error);
      illegal = true;
    }
    ASSERT(illegal);
  }
  {
    bool illegal = false;
    try {
      TEST(lighting->stores(~specular) >> render_pass[~specular]->stores(output));  // illegal; makes no sense to clear an attachment that was just written to.
    } catch (AIAlert::Error const& error) {
      Dout(dc::renderpass, "Illegal: " << error);
      illegal = true;
    }
    ASSERT(illegal);
  }
  {
    TEST(lighting->stores(~specular) >> render_pass->stores(specular));             // { LOAD, STORE }
    render_graph.has_with(in|out, specular, LOAD, STORE);
  }
  {
    bool illegal = false;
    try {
      TEST(lighting->stores(~specular) >> render_pass[-specular]->stores(specular));// illegal; makes no sense to write to an attachment that was just written to.
    } catch (AIAlert::Error const& error) {
      Dout(dc::renderpass, "Illegal: " << error);
      illegal = true;
    }
    ASSERT(illegal);
  }
  {
    bool illegal = false;
    try {
      TEST(lighting->stores(~specular) >> render_pass[+specular]->stores(specular));// { LOAD, STORE } - but over specified. Lets not accept this.
    } catch (AIAlert::Error const& error) {
      Dout(dc::renderpass, "Illegal: " << error);
      illegal = true;
    }
    ASSERT(illegal);
  }
  {
    bool illegal = false;
    try {
      TEST(lighting->stores(~specular) >> render_pass[~specular]->stores(specular));// illegal; makes no sense to clear an attachment that was just written to.
    } catch (AIAlert::Error const& error) {
      Dout(dc::renderpass, "Illegal: " << error);
      illegal = true;
    }
    ASSERT(illegal);
  }
  {
    bool illegal = false;
    try {
      TEST(lighting->stores(~specular) >> render_pass->stores(~specular));          // illegal; makes no sense to clear an attachment that was just written to.
    } catch (AIAlert::Error const& error) {
      Dout(dc::renderpass, "Illegal: " << error);
      illegal = true;
    }
    ASSERT(illegal);
  }
  {
    bool illegal = false;
    try {
      TEST(lighting->stores(~specular) >> render_pass[-specular]->stores(~specular));// illegal; makes no sense to clear an attachment that was just written to.
    } catch (AIAlert::Error const& error) {
      Dout(dc::renderpass, "Illegal: " << error);
      illegal = true;
    }
    ASSERT(illegal);
  }
  {
    bool illegal = false;
    try {
      TEST(lighting->stores(~specular) >> render_pass[+specular]->stores(~specular));// illegal; makes no sense to clear an attachment that was just written to.
    } catch (AIAlert::Error const& error) {
      Dout(dc::renderpass, "Illegal: " << error);
      illegal = true;
    }
    ASSERT(illegal);
  }
  {
    bool illegal = false;
    try {
      TEST(lighting->stores(~specular) >> render_pass[~specular]->stores(~specular));// illegal; makes no sense to clear an attachment that was just written to.
    } catch (AIAlert::Error const& error) {
      Dout(dc::renderpass, "Illegal: " << error);
      illegal = true;
    }
    ASSERT(illegal);
  }
  {
    TEST(lighting->stores(~specular) >> render_pass->stores(output) >> shadow[+specular]->stores(output));
    render_graph.has_with(in, specular, LOAD, STORE);
  }

  DoutFatal(dc::fatal, "RenderGraph::testuite successful!");
}

void RenderGraph::has_with(char, Attachment const& attachment) const
{
  ASSERT(m_sinks.size() == 1);
  RenderPass const* render_pass = m_sinks[0];
  // Attachment not used.
  ASSERT(!render_pass->is_known(attachment));
  // Nothing should be listen in m_remove_or_dontcare_attachments.
  ASSERT(render_pass->remove_or_dontcare_attachments_empty());
}

void RenderGraph::has_with(int in_out, Attachment const& attachment, int required_load_op, int required_store_op) const
{
  ASSERT(m_sinks.size() == 1);
  // The render pass under test is always the last render pass.
  RenderPass const* render_pass = m_sinks[0];

  // Nothing should be listen in m_remove_or_dontcare_attachments.
  ASSERT(render_pass->remove_or_dontcare_attachments_empty());

  // Same as RenderGraph::testsuite.
  constexpr int in = 1;
  constexpr int out = 2;
  constexpr int internal = 4;

  constexpr int DONT_CARE = 0;
  constexpr int LOAD = 1;
  constexpr int CLEAR = 2;
  constexpr int STORE = 3;

  // Use `none` for the zero value.
  ASSERT(in_out);
  // Only use `internal` on its own.
  ASSERT(in_out == internal || !static_cast<bool>(in_out & internal));

  auto load_op = render_pass->get_load_op(attachment);
  auto store_op = render_pass->get_store_op(attachment);

  vk::AttachmentLoadOp rq_load_op = required_load_op == LOAD ? vk::AttachmentLoadOp::eLoad : required_load_op == CLEAR ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eDontCare;
  vk::AttachmentStoreOp rq_store_op = required_store_op == STORE ? vk::AttachmentStoreOp::eStore : vk::AttachmentStoreOp::eDontCare;

  ASSERT(load_op == rq_load_op);
  ASSERT(store_op == rq_store_op);
}

#endif // CWDEBUG

} // namespace vulkan::rendergraph

#if !defined(DOXYGEN)
NAMESPACE_DEBUG_CHANNELS_START
channel_ct renderpass("RENDERPASS");
NAMESPACE_DEBUG_CHANNELS_END
#endif
