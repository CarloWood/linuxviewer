#pragma once

#include "rendergraph/RenderPass.h"

namespace vulkan {

class RenderPass : public rendergraph::RenderPass
{
 public:
  // Constructor (only construct RenderPass nodes as objects in your Window class.
  //
  // For example, use:
  //
  // class MyWindow : public task::SynchronousWindow {
  //   RenderPass main_pass{this, "main_pass"};
  //
  RenderPass(task::SynchronousWindow* owning_window, std::string const& name);
};

} // namespace vulkan
