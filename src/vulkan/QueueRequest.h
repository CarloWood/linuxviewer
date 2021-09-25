#pragma once

#include "QueueFlags.h"
#include "utils/Vector.h"
#include <cstdint>
#ifdef CWDEBUG
#include <iosfwd>
#endif

namespace vulkan {

struct QueueRequest
{
  QueueFlags queue_flags;               // Request a queue with ALL of the flags.
  uint32_t max_number_of_queues = 0;    // Allocate at most this many queues like that. Zero means: as many as supported by the device.
  float priority = 1.0;                 // Requested relative priority for those queues.

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

// An index for Vectors containing QueueRequest's or QueueReply's.
using QueueRequestIndex = utils::VectorIndex<QueueRequest>;

} // namespace vulkan
