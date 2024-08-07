#pragma once
#undef LV_NEEDS_FLAT_CREATE_INFO_INL_H

#include "CharacteristicRange.h"
#include "xml/Bridge.h"

namespace vulkan::pipeline {

template<typename T>
//static
std::vector<T> FlatCreateInfo::merge(threadsafe::Unlocked<utils::Vector<CharacteristicDataCache<T>, CharacteristicRangeIndex>, threadsafe::policy::Primitive<std::mutex>>& input_list, characteristics_container_t const& characteristics)
{
  std::vector<T> result;
  typename std::remove_reference_t<decltype(input_list)>::wat input_list_w(input_list);
  for (CharacteristicRangeIndex characteristic_range_index = input_list_w->ibegin();
      characteristic_range_index != input_list_w->iend(); ++characteristic_range_index)
  {
    // Was this vector added to the flat create info by this characteristic range?
    std::vector<T> const* vec_ptr = (*input_list_w)[characteristic_range_index].m_characteristic_data;
    if (vec_ptr)
    {
      // Get a reference to the cache vector.
      std::vector<std::vector<T>>& per_fill_index_cache((*input_list_w)[characteristic_range_index].m_per_fill_index_cache);
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

#if CW_DEBUG
      // You called add(std::vector<T> const&) but never filled the passed vector with data.
      //
      // For example,
      //
      // T = vk::VertexInputBindingDescription:
      // if you do not have any vertex buffers then you should set m_use_vertex_buffers = false; in
      // the constructor of the pipeline factory characteristic derived from AddVertexShader.
      if (vec_ptr->empty())
      {
        if constexpr (std::is_same_v<T, vk::VertexInputBindingDescription>)
          DoutFatal(dc::core, "If you do not have any vertex buffers then you should set m_use_vertex_buffers = false; in the constructor of the pipeline factory characteristic derived from AddVertexShader.");
        else if constexpr (std::is_same_v<T, vk::VertexInputBindingDescription>)
          DoutFatal(dc::core, "The factory characteristic is derived from AddVertexShader, but m_vertex_input_binding_descriptions was never filled.");
        else if constexpr (std::is_same_v<T, vk::VertexInputAttributeDescription>)
          DoutFatal(dc::core, "The factory characteristic is derived from AddVertexShader, but m_vertex_input_attribute_descriptions was never filled.");
        else if constexpr (std::is_same_v<T, PushConstantRange>)
          DoutFatal(dc::core, "The factory characteristic is derived from AddPushConstant, but m_push_constant_ranges was never filled.");
        else if constexpr (std::is_same_v<T, vk::PipelineShaderStageCreateInfo>)
          DoutFatal(dc::core, "The factory characteristic is derived from AddShaderStage, but m_shader_stage_create_infos was never filled. Did you forget to call add_shader() in your factory characteristic class?");
        else
          DoutFatal(dc::core, "You called FlatCreateInfo::add(std::vector<" <<
            libcwd::type_info_of<T>().demangled_name() << "> const&) but never filled the passed vector with data.");
      }
#endif
      result.insert(result.end(), vec_ptr->begin(), vec_ptr->end());
    }
  }
  return result;
}

template<typename T>
void CharacteristicDataCache<T>::xml(xml::Bridge& xml)
{
  xml.node_name("CharacteristicDataCache");
  if (xml.writing())
  {
    if (m_characteristic_data)
    {
      xml.children("m_characteristic_data", *const_cast<std::vector<T>*>(m_characteristic_data));
      xml.children("m_per_fill_index_cache", m_per_fill_index_cache);
    }
  }
}

} // namespace vulkan::pipeline
