#include "sys.h"
#include "FactoryHandle.h"
#include "SynchronousWindow.h"

namespace vulkan::pipeline {

void FactoryHandle::generate(task::SynchronousWindow const* owning_window)
{
  DoutEntering(dc::vulkan, "pipeline::FactoryHandle::generate(" << owning_window << ")");
  owning_window->pipeline_factory(m_factory_index)->generate();
}

} // namespace vulkan::pipeline
