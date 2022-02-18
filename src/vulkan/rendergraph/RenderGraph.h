#pragma once

#include "RenderPass.h"
#include "ClearValue.h"
#include <map>

namespace vulkan::rendergraph {

// RenderGraph
//
// Object representing a whole render graph.
// Has pointers to both, source and sink leaves.
//
class RenderGraph
{
 private:
  std::vector<RenderPass*> m_sources;                   // All leaves that have only outgoing vertices (finalized by generate()).
  std::vector<RenderPass*> m_sinks;                     // All leaves that have only incoming vertices (finalized by generate()).

  mutable int m_traversal_id = {};                      // Unique ID to identify which RenderPass nodes have already visited.
                                                        // Incremented every call to for_each_render_pass.
  bool m_have_incoming_outgoing = false;                // Set to true after m_sources was fixed to point to real sources and all RenderPass nodes have correct m_outgoing_vertices.

 public:
  // Filled by SynchronousWindow.
  ClearValue m_default_color_clear_value;                       // Clear value that is used for color attachments by default (if they are cleared).
  ClearValue m_default_depth_stencil_clear_value{1.f, 0};       // Clear value that is used for depth/stencil attachments by default (if they are cleared).

 public:
  void operator=(RenderPassStream& sink);
  void operator+=(RenderPassStream& sink);
  enum Direction { search_backwards, search_forwards };
  void for_each_render_pass(Direction direction, std::function<bool(RenderPass*, std::vector<RenderPass*>&)> lambda) const;
  void for_each_render_pass_from(RenderPass* start, Direction direction, std::function<bool(RenderPass*, std::vector<RenderPass*>&)> lambda) const;
  void generate(task::SynchronousWindow* owning_window);

#ifdef CWDEBUG
  // Testsuite stuff.
  static void testsuite();
  void has_with(char, Attachment const& attachment) const;
  void has_with(int in_out, Attachment const& a, int required_load_op, int required_store_op, RenderPass const* render_pass = nullptr) const;
  static std::string to_string(Direction direction);
  friend std::ostream& operator<<(std::ostream& os, Direction direction) { return os << to_string(direction); }
#endif
};

} // namespace vulkan::rendergraph
