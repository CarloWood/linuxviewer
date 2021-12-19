#ifndef RENDER_PASS_STREAM_H
#define RENDER_PASS_STREAM_H

#ifdef CWDEBUG
#include "debug.h"
#include <iosfwd>
#endif

namespace vulkan::rendergraph {

class RenderPass;

// RenderPassStream
//
// Helper class to ease the use of operator-> (which just looks better).
// In other words, to be able to write
//
//   pass1[a2]->stores(a1)
//
// rather than
//
//   pass1[a2].stores(a1)
//
// It also enforces the usage of `stores` before operator>> can be used.
//
struct RenderPassStream
{
 private:
  RenderPass* m_owner;                                                  // The RenderPass that contains this RenderPassStream object.
  RenderPassStream* m_preceding_render_pass = nullptr;                  // A double linked list of render passes that were coupled using operator<<.
  RenderPassStream* m_subsequent_render_pass = nullptr;                 // Aka, with pass1->stores(a1) >> pass2->stores(...) then pass1.m_subsequent_render_pass
                                                                        // points to pass2 and pass2.m_preceding_render_pass points to pass1.
 public:
  RenderPassStream(RenderPass* owner) : m_owner(owner) { }

  template<typename Arg1, typename... Args>
  RenderPassStream& stores(Arg1 const& arg1, Args const&... args);

  // Accessors.
  RenderPass* owner() { return m_owner; }
  RenderPassStream* preceding_render_pass() const { return m_preceding_render_pass; }
  RenderPassStream* subsequent_render_pass() const { return m_subsequent_render_pass; }

  RenderPassStream& get_source()
  {
    RenderPassStream* source = this;
    // Find the source.
    while (source->m_preceding_render_pass)
      source = source->m_preceding_render_pass;
    return *source;
  }

  void stores_called(RenderPass* render_pass);

  RenderPassStream& operator>>(RenderPassStream& subsequent_render_pass);
  void link(RenderPassStream& subsequent_render_pass);
  inline void do_load_dont_cares();

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
  friend std::ostream& operator<<(std::ostream& os, RenderPassStream const& render_pass_stream) { render_pass_stream.print_on(os); return os; }
#endif
};

} // namespace vulkan::rendergraph

#include "RenderPass.h"

namespace vulkan::rendergraph {

template<typename Arg1, typename... Args>
RenderPassStream& RenderPassStream::stores(Arg1 const& arg1, Args const&... args)
{
  Dout(dc::renderpass, m_owner << "->stores(" << arg1 << (sizeof...(args) == 0 ? "" : ", ") << join(", ", args...) << ")");
  stores_called(m_owner);
  m_owner->store_attachment(arg1);
  (m_owner->store_attachment(args), ...);
  return *this;
}

void RenderPassStream::do_load_dont_cares()
{
  m_owner->do_load_dont_cares();
}

} // namespace vulkan::rendergraph

#endif // RENDER_PASS_STREAM_H
