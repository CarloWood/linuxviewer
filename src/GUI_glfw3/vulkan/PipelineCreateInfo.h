#pragma once

#include "debug.h"

namespace vulkan {

struct PipelineCreateInfo
{
  void print_on(std::ostream& os) const;
};

} // namespace vulkan
