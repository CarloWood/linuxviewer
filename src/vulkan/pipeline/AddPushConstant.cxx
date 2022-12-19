#include "sys.h"
#include "AddPushConstant.h"
#include "PipelineFactory.h"
#include <algorithm>
#include <iterator>

namespace vulkan::pipeline {

void AddPushConstant::copy_push_constant_ranges(task::PipelineFactory* pipeline_factory)
{
  DoutEntering(dc::vulkan, "AddPushConstant::copy_push_constant_ranges(" << pipeline_factory << ")");
  // Copy the data that was added by calls to insert_push_constant_range if it was changed since the last time we copied it.
  m_push_constant_ranges.clear();
  if (m_sorted_push_constant_ranges_changed)
  {
    std::copy(m_sorted_push_constant_ranges.begin(), m_sorted_push_constant_ranges.end(), std::back_inserter(m_push_constant_ranges));
    m_sorted_push_constant_ranges_changed = false;
  }
}

void AddPushConstant::register_AddPushConstant_with(task::PipelineFactory* pipeline_factory) const
{
  pipeline_factory->add_to_flat_create_info(&m_push_constant_ranges);
}

} // namespace vulkan::pipeline
