#pragma once

#include "SetKey.h"
#include "SetIndex.h"
#include <map>

namespace vulkan::descriptor {

class SetKeyToSetIndex
{
 private:
  std::map<SetKey, SetIndex> m_set_key_to_set_index;
};

} // namespace vulkan::identifier
