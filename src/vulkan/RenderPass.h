#pragma once

#include "rendergraph/RenderPass.h"

namespace vulkan {

class RenderPass : public rendergraph::RenderPass
{
 private:
  task::SynchronousWindow* m_owning_window;
  vk::UniqueRenderPass m_render_pass;           // The underlaying Vulkan render pass.

 public:
  // Constructor (only construct RenderPass nodes as objects in your Window class.
  //
  // For example, use:
  //
  // class MyWindow : public task::SynchronousWindow {
  //   RenderPass main_pass{this, "main_pass"};
  //
  RenderPass(task::SynchronousWindow* owning_window, std::string const& name);

  // Called from rendergraph::generate after creating the render pass.
  void set_render_pass(vk::UniqueRenderPass&& render_pass) { m_render_pass = std::move(render_pass); }

  // Return a vector with clear values for this RenderPass that
  // can be used for vk::RenderPassBeginInfo::pClearValues.
  std::vector<vk::ClearValue> clear_values() const;

  // Accessors.
  vk::RenderPass vh_render_pass() const
  {
    return *m_render_pass;
  }

 private:
  void create_render_pass() override;
};

} // namespace vulkan
