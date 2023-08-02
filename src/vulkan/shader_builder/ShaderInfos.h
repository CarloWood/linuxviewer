#pragma once

#include "ShaderInfo.h"
#include "ShaderIndex.h"
#include "ShaderResourceDeclaration.h"
#include "../PushConstantRange.h"
#include "../descriptor/SetIndexHintMap.h"
#include "threadsafe/threadsafe.h"
#include "statefultask/AIStatefulTaskMutex.h"
#include "utils/Deque.h"
#include <map>

namespace vulkan::shader_builder {

struct ShaderInfoCache : ShaderInfo
{
  AIStatefulTaskMutex m_task_mutex;                     // A task mutex that should be locked while accessing m_shader_module.
  vk::UniqueShaderModule m_shader_module;               // The shader module that corresponds to the ShaderIndex at which it is stored.

  // Cache of AddPushConstant::m_push_constant_ranges.
  std::vector<PushConstantRange> m_push_constant_ranges;
  // Cache of AddVertexShader::m_vertex_input_*_descriptions.
  std::vector<vk::VertexInputBindingDescription> m_vertex_input_binding_descriptions;
  std::vector<vk::VertexInputAttributeDescription> m_vertex_input_attribute_descriptions;
  // Cache of the relevant part (the DescriptorSetLayoutBinding base class) of the ShaderResourceDeclaration's used by m_stage.
  std::vector<DescriptorSetLayoutBinding> m_descriptor_set_layouts;
  // Cache of AddShaderStage::m_set_index_hint_map2.
  descriptor::SetIndexHintMap m_set_index_hint_map4;

  // Moving is done during pre-initialization, like from register_shader_templates;
  // therefore we can ignore the mutex as well as m_shader_module (which will be NULL anyway).
  ShaderInfoCache(ShaderInfo&& shader_info) : ShaderInfo(std::move(shader_info)) { }

  void copy(descriptor::SetIndexHintMap const* set_index_hint_map2)
  {
    ASSERT(m_set_index_hint_map4.empty());
    m_set_index_hint_map4 = *set_index_hint_map2;
  }

  void copy(std::vector<PushConstantRange> const& push_constant_ranges)
  {
    ASSERT(m_push_constant_ranges.empty());
    m_push_constant_ranges = push_constant_ranges;
  }

  void copy(std::vector<vk::VertexInputBindingDescription> const& vertex_input_binding_descriptions,
      std::vector<vk::VertexInputAttributeDescription> const& vertex_input_attribute_descriptions)
  {
    ASSERT(m_vertex_input_binding_descriptions.empty() && m_vertex_input_attribute_descriptions.empty());
    m_vertex_input_binding_descriptions = vertex_input_binding_descriptions;
    m_vertex_input_attribute_descriptions = vertex_input_attribute_descriptions;
  }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

// Helper class used by Application to store all ShaderInfo instances.
struct UnlockedShaderInfos
{
  utils::Deque<ShaderInfoCache, ShaderIndex> deque;     // All ShaderInfoCache objects. This must be a deque because it is grown
                                                        // after already handing out indices into it.
  std::map<std::size_t, ShaderIndex> hash_to_index;     // A map from ShaderInfo hash values to their index into deque.
};

using ShaderInfos = threadsafe::Unlocked<UnlockedShaderInfos, threadsafe::policy::Primitive<std::mutex>>;

} // namespace vulkan::shader_builder
