#include "sys.h"
#include "AddPushConstant.h"
#include "PipelineFactory.h"
#include "shader_builder/ShaderInfos.h"
#include <algorithm>
#include <iterator>

namespace vulkan::pipeline {

void AddPushConstant::register_AddPushConstant_with(task::PipelineFactory* pipeline_factory, task::CharacteristicRange const& characteristic_range) const
{
  pipeline_factory->add_to_flat_create_info(&m_push_constant_ranges, characteristic_range);
}

void AddPushConstant::copy_push_constant_ranges(task::PipelineFactory* pipeline_factory)
{
  DoutEntering(dc::vulkan, "AddPushConstant::copy_push_constant_ranges(" << pipeline_factory << ")");
  // Copy the data that was added by calls to insert_push_constant_range if it was changed since the last time we copied it.
  if (m_sorted_push_constant_ranges_changed)
  {
    m_push_constant_ranges.clear();
    std::copy(m_sorted_push_constant_ranges.begin(), m_sorted_push_constant_ranges.end(), std::back_inserter(m_push_constant_ranges));
    m_sorted_push_constant_ranges_changed = false;
  }
}

void AddPushConstant::cache_push_constant_ranges(shader_builder::ShaderInfoCache& shader_info_cache)
{
  shader_info_cache.copy(m_push_constant_ranges);
}

void AddPushConstant::restore_push_constant_ranges(shader_builder::ShaderInfoCache const& shader_info_cache)
{
  if (m_push_constant_ranges.empty())
    m_push_constant_ranges = shader_info_cache.m_push_constant_ranges;
#if CW_DEBUG
  else
  {
    // If we DID already call preprocess1, then we should have gotten the same push constant ranges as another pipeline factory of course!
    ASSERT(m_push_constant_ranges == shader_info_cache.m_push_constant_ranges);
  }
#endif
}

} // namespace vulkan::pipeline
