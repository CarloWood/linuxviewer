#pragma once

#include "ShaderInfo.h"
#include "ShaderIndex.h"
#include "threadsafe/aithreadsafe.h"
#include "utils/Deque.h"
#include <map>

namespace vulkan::shaderbuilder {

// Helper class used by Application to store all ShaderInfo instances.
struct UnlockedShaderInfos
{
  utils::Deque<ShaderInfo, ShaderIndex> deque;          // All ShaderInfo objects. This must be a deque because it is grown after already handing out indices into it.
  std::map<std::size_t, ShaderIndex> hash_to_index;     // A map from ShaderInfo hash values to their index into list.
};

using ShaderInfos = aithreadsafe::Wrapper<UnlockedShaderInfos, aithreadsafe::policy::Primitive<std::mutex>>;

} // namespace vulkan::shaderbuilder
