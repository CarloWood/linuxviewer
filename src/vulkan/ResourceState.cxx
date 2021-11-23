#ifdef CWDEBUG
#include "sys.h"
#include "ResourceState.h"
#include "vk_utils/print_flags.h"
#include <iostream>

namespace vulkan {

void ResourceState::print_on(std::ostream& os) const
{
  os << "{ pipeline_stage_mask:" << pipeline_stage_mask <<
        ", access_mask:" << access_mask <<
        ", layout:" << layout <<
        ", queue_family_index:" << queue_family_index <<
        '}';
}

} // namespace vulkan
#endif
