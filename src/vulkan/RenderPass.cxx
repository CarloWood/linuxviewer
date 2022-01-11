#include "sys.h"
#include "RenderPass.h"
#include "SynchronousWindow.h"

namespace vulkan {

RenderPass::RenderPass(task::SynchronousWindow* owning_window, std::string const& name) : rendergraph::RenderPass(name)
{
  owning_window->register_render_pass(this);
}

} // namespace vulkan
