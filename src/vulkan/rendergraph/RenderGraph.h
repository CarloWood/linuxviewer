#pragma once

#include "RenderPass.h"

namespace vulkan::rendergraph {

// RenderGraph
//
// Object representing a whole render graph.
// Has pointers to both, source and sink leaves.
//
class RenderGraph
{
 private:
  std::vector<RenderPass*> m_sources;
  std::vector<RenderPass*> m_sinks;

 public:
  void operator=(RenderPassStream& sink);
  void create();

#ifdef CWDEBUG
  // Testsuite stuff.
  static void testsuite();
  void has_with(char, Attachment const& a) const;
  void has_with(int in_out, Attachment const& a, int required_load_op, int required_store_op) const;
#endif
};

} // namespace vulkan::rendergraph
