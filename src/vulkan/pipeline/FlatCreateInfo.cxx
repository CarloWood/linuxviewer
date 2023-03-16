#include "sys.h"
#include "FlatCreateInfo.h"
#include "CharacteristicRange.h"

#include "FlatCreateInfo.inl.h"

namespace vulkan::pipeline {

void FlatCreateInfo::increment_number_of_characteristics()
{
  pipeline_shader_stage_create_infos_list_t::wat(m_pipeline_shader_stage_create_infos_list)->emplace_back();
  vertex_input_binding_descriptions_list_t::wat(m_vertex_input_binding_descriptions_list)->emplace_back();
  vertex_input_attribute_descriptions_list_t::wat(m_vertex_input_attribute_descriptions_list)->emplace_back();
  pipeline_color_blend_attachment_states_list_t::wat(m_pipeline_color_blend_attachment_states_list)->emplace_back();
  dynamic_states_list_t::wat(m_dynamic_states_list)->emplace_back();
  push_constant_ranges_list_t::wat(m_push_constant_ranges_list)->emplace_back();
}

void FlatCreateInfo::add(std::vector<vk::PipelineShaderStageCreateInfo> const* pipeline_shader_stage_create_infos, task::CharacteristicRange const& owning_characteristic_range)
{
  pipeline_shader_stage_create_infos_list_t::wat pipeline_shader_stage_create_infos_list_w(m_pipeline_shader_stage_create_infos_list);
  CharacteristicRangeIndex characteristic_range_index = owning_characteristic_range.characteristic_range_index();
  // Added vectors must have a size that corresponds to the size of the range.
  // Did you call FactoryHandle::generate(window) on the handle returned by SynchronousWindow::create_pipeline_factory?
  ASSERT(characteristic_range_index < pipeline_shader_stage_create_infos_list_w->iend());
  (*pipeline_shader_stage_create_infos_list_w)[characteristic_range_index].m_characteristic_data = pipeline_shader_stage_create_infos;
}

void FlatCreateInfo::add(std::vector<vk::VertexInputBindingDescription> const* vertex_input_binding_descriptions, task::CharacteristicRange const& owning_characteristic_range)
{
  vertex_input_binding_descriptions_list_t::wat vertex_input_binding_descriptions_list_w(m_vertex_input_binding_descriptions_list);
  CharacteristicRangeIndex characteristic_range_index = owning_characteristic_range.characteristic_range_index();
  // Added vectors must have a size that corresponds to the size of the range.
  // Did you call FactoryHandle::generate(window) on the handle returned by SynchronousWindow::create_pipeline_factory?
  ASSERT(characteristic_range_index < vertex_input_binding_descriptions_list_w->iend());
  (*vertex_input_binding_descriptions_list_w)[characteristic_range_index].m_characteristic_data = vertex_input_binding_descriptions;
}

void FlatCreateInfo::add(std::vector<vk::VertexInputAttributeDescription> const* vertex_input_attribute_descriptions, task::CharacteristicRange const& owning_characteristic_range)
{
  vertex_input_attribute_descriptions_list_t::wat vertex_input_attribute_descriptions_list_w(m_vertex_input_attribute_descriptions_list);
  CharacteristicRangeIndex characteristic_range_index = owning_characteristic_range.characteristic_range_index();
  // Added vectors must have a size that corresponds to the size of the range.
  // Did you call FactoryHandle::generate(window) on the handle returned by SynchronousWindow::create_pipeline_factory?
  ASSERT(characteristic_range_index < vertex_input_attribute_descriptions_list_w->iend());
  (*vertex_input_attribute_descriptions_list_w)[characteristic_range_index].m_characteristic_data = vertex_input_attribute_descriptions;
}

void FlatCreateInfo::add(std::vector<vk::PipelineColorBlendAttachmentState> const* pipeline_color_blend_attachment_states, task::CharacteristicRange const& owning_characteristic_range)
{
  pipeline_color_blend_attachment_states_list_t::wat pipeline_color_blend_attachment_states_list_w(m_pipeline_color_blend_attachment_states_list);
  CharacteristicRangeIndex characteristic_range_index = owning_characteristic_range.characteristic_range_index();
  // Added vectors must have a size that corresponds to the size of the range.
  // Did you call FactoryHandle::generate(window) on the handle returned by SynchronousWindow::create_pipeline_factory?
  ASSERT(characteristic_range_index < pipeline_color_blend_attachment_states_list_w->iend());
  (*pipeline_color_blend_attachment_states_list_w)[characteristic_range_index].m_characteristic_data = pipeline_color_blend_attachment_states;
}

void FlatCreateInfo::add(std::vector<vk::DynamicState> const* dynamic_states, task::CharacteristicRange const& owning_characteristic_range)
{
  dynamic_states_list_t::wat dynamic_states_list_w(m_dynamic_states_list);
  CharacteristicRangeIndex characteristic_range_index = owning_characteristic_range.characteristic_range_index();
  // Added vectors must have a size that corresponds to the size of the range.
  // Did you call FactoryHandle::generate(window) on the handle returned by SynchronousWindow::create_pipeline_factory?
  ASSERT(characteristic_range_index < dynamic_states_list_w->iend());
  (*dynamic_states_list_w)[characteristic_range_index].m_characteristic_data = dynamic_states;
}

void FlatCreateInfo::add(std::vector<vulkan::PushConstantRange> const* push_constant_ranges, task::CharacteristicRange const& owning_characteristic_range)
{
  push_constant_ranges_list_t::wat push_constant_ranges_list_w(m_push_constant_ranges_list);
  CharacteristicRangeIndex characteristic_range_index = owning_characteristic_range.characteristic_range_index();
  // Added vectors must have a size that corresponds to the size of the range.
  // Did you call FactoryHandle::generate(window) on the handle returned by SynchronousWindow::create_pipeline_factory?
  ASSERT(characteristic_range_index < push_constant_ranges_list_w->iend());
  (*push_constant_ranges_list_w)[characteristic_range_index].m_characteristic_data = push_constant_ranges;
}

std::vector<vk::PushConstantRange> FlatCreateInfo::realize_sorted_push_constant_ranges(characteristics_container_t const& characteristics)
{
  DoutEntering(dc::vulkan, "FlatCreateInfo::realize_sorted_push_constant_ranges() [" << this << "]");

  // sorted_push_constant_ranges is sorted on (from highest to lowest priority):
  // - push constant type (aka, MyPushConstant1, MyPushConstant2, etc).
  // - vk::ShaderStageFlags
  // - offset
  //
  // Merging then works as follows:
  // 1) each push constant type is unique, and just catenated in no particular order (lets say in the order
  //    of whatever the id is that we assign to each type).
  // 2) ranges for a given vk::ShaderStageFlags are extended (offset and size)
  // 3) the new offsets determine the order in which the ranges for each vk::ShaderStageFlags are stored.

  // Original input.
  push_constant_ranges_list_t::wat push_constant_ranges_list_w(m_push_constant_ranges_list);

#if CWDEBUG
  Dout(dc::vulkan, "m_push_constant_ranges_list = ");
  CharacteristicRangeIndex i{0};
  for (auto const& characteristic_data_cache : *push_constant_ranges_list_w)
  {
    std::vector<vulkan::PushConstantRange> const* vec_ptr = characteristic_data_cache.m_characteristic_data;
    if (vec_ptr)
    {
      Dout(dc::vulkan, "  ranges " << i << ":");
      for (vulkan::PushConstantRange const& range : *vec_ptr)
        Dout(dc::vulkan, "    " << range);
    }
    ++i;
  }
#endif

  // If there is no input data, return an empty vector.
  int number_of_input_vectors =
    std::count_if(push_constant_ranges_list_w->begin(), push_constant_ranges_list_w->end(),
        [](CharacteristicDataCache<vulkan::PushConstantRange> const& characteristic_data_cache) { return characteristic_data_cache.m_characteristic_data != nullptr; });
  if (number_of_input_vectors == 0)
    return {};

  // Intermediate vectors.
  using input_vectors_list_t = std::vector<std::vector<vulkan::PushConstantRange>>;
  std::array<input_vectors_list_t, 2> storage;

  // Copy the input vectors from m_push_constant_ranges_list into storage[0].
  storage[0].resize(number_of_input_vectors);
  int input_vector = 0;
  for (CharacteristicRangeIndex characteristic_range_index = push_constant_ranges_list_w->ibegin();
      characteristic_range_index != push_constant_ranges_list_w->iend(); ++characteristic_range_index)
  {
    // Was this vector added to the flat create info by this characteristic range?
    std::vector<vulkan::PushConstantRange> const* vec_ptr = (*push_constant_ranges_list_w)[characteristic_range_index].m_characteristic_data;
    if (vec_ptr)
    {
      // Get a reference to the cache vector.
      std::vector<std::vector<vulkan::PushConstantRange>>&
        per_fill_index_cache((*push_constant_ranges_list_w)[characteristic_range_index].m_per_fill_index_cache);
      int number_cached_values = per_fill_index_cache.size();

      // Get the current fill_index.
      // If fill_index == -1 then vec_ptr was added by a Characteristic and we should use slot 0.
      int fill_index = std::max(0, characteristics[characteristic_range_index]->fill_index());

      // Already cached?
      if (fill_index < number_cached_values)
      {
        // Use the cached vector.
        vec_ptr = &per_fill_index_cache[fill_index];
      }
      else
      {
        // The fill_index is expected to run from 0 till the end of the range with increments of one.
        ASSERT(per_fill_index_cache.size() == fill_index);
        // Add vec_ptr to the cache.
        per_fill_index_cache.emplace_back(vec_ptr->begin(), vec_ptr->end());
      }

      std::copy(vec_ptr->begin(), vec_ptr->end(), std::back_inserter(storage[0][input_vector]));
      ++input_vector;
    }
  }

  input_vectors_list_t const* input = &storage[0];
  input_vectors_list_t* output = &storage[1];
  int s = 1;  // The index into storage that output is pointing at.

  for (;;)
  {
    auto in_vector1 = input->begin();         // The next vector in the input that wasn't merged yet.
    size_t need_merge = input->size();        // Number of vectors in input that still need merging.
    do
    {
      // The next output vector.
      if (need_merge == 1)
      {
        output->push_back(std::move(*in_vector1));
        break;
      }
      else
      {
        output->emplace_back();
        auto in_vector2 = in_vector1++;
        std::set_union(in_vector2->begin(), in_vector2->end(),
                       in_vector1->begin(), in_vector1->end(),
                       std::back_inserter(output->back()));
        need_merge -= 2;
        ++in_vector1;
      }
    }
    while (need_merge > 0);

    // If everything was merged into a single vector then we're done.
    if (output->size() == 1)
      break;

    input = output;
    s = 1 - s;
    output = &storage[s];
    output->clear();
  }

  // Make a reference to the consolidated data in output.
  std::vector<vulkan::PushConstantRange> const& consolidated_ranges = (*output)[0];

  std::vector<vk::PushConstantRange> merged_result;

  std::type_index last_type_index = typeid(NoPushConstant);
  vk::ShaderStageFlags last_shader_stage_flags{};
#ifdef CWDEBUG
  int last_gap = 0;
  int last_size = 1;
#endif

  for (auto in = consolidated_ranges.begin(); in != consolidated_ranges.end(); ++in)
  {
    if (in->type_index() != last_type_index || in->shader_stage_flags() != last_shader_stage_flags)
    {
#ifdef CWDEBUG
      int unused_gap = 100 * last_gap / last_size;
      if (unused_gap > 20)
        Dout(dc::warning, "Push constant range is nonconsecutive (" << unused_gap << "%% is unused).");
      // Please group the members of your PushConstant structs so that those that are used in the same stage(s) are together.
      ASSERT(unused_gap < 50);
#endif
      merged_result.push_back({in->shader_stage_flags(), in->offset(), in->size()});
      last_type_index = in->type_index();
      last_shader_stage_flags = in->shader_stage_flags();
#ifdef CWDEBUG
      last_gap = 0;
      last_size = in->size();
#endif
    }
    else
    {
      vk::PushConstantRange& last_result = merged_result.back();
      // Note: no need to update last_result.offset because consolidated_ranges was sorted on offset already,
      // therefore last_result.offset will be the smallest value already.
      //
#ifdef CWDEBUG
      // The resulting range only becomes nonconsecutive if the next range has an offset that is beyond the current range.
      last_gap += std::max(uint32_t{0}, in->offset() - (last_result.offset + last_result.size));
#endif
      last_result.size = std::max(last_result.offset + last_result.size, in->offset() + in->size()) - last_result.offset;
#ifdef CWDEBUG
      last_size = last_result.size;
#endif
    }
  }

  return merged_result;
}

} // namespace vulkan::pipeline
