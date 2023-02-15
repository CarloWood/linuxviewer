#pragma once

#include "ShaderInfo.h"
#include "ShaderIndex.h"
#include "threadsafe/aithreadsafe.h"
#include "statefultask/AIStatefulTaskMutex.h"
#include "utils/Deque.h"
#include <map>

namespace vulkan::shader_builder {

struct ShaderInfoCache : ShaderInfo
{
  AIStatefulTaskMutex m_task_mutex;                     // A task mutex that should be locked while accessing m_shader_module.
  vk::UniqueShaderModule m_shader_module;               // The shader module that corresponds to the ShaderIndex at which it is stored.

  // Moving is done during pre-initialization, like from register_shader_templates;
  // therefore we can ignore the mutex as well as m_shader_module (which will be NULL anyway).
  ShaderInfoCache(ShaderInfo&& shader_info) : ShaderInfo(std::move(shader_info)) { }
};

// Helper class used by Application to store all ShaderInfo instances.
struct UnlockedShaderInfos
{
  utils::Deque<ShaderInfoCache, ShaderIndex> deque;     // All ShaderInfoCache objects. This must be a deque because it is grown
                                                        // after already handing out indices into it.
  std::map<std::size_t, ShaderIndex> hash_to_index;     // A map from ShaderInfo hash values to their index into deque.
};

using ShaderInfos = aithreadsafe::Wrapper<UnlockedShaderInfos, aithreadsafe::policy::Primitive<std::mutex>>;

} // namespace vulkan::shader_builder
