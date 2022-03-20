#include "sys.h"
#include "Handle.h"
#include "SynchronousWindow.h"

namespace vulkan::pipeline {

void Handle::generate(task::SynchronousWindow const* owning_window)
{
  DoutEntering(dc::vulkan, "pipeline::Handle::generate(" << owning_window << ")");
  owning_window->pipeline_factory(m_factory_index)->generate();
}

} // namespace vulkan::pipeline
