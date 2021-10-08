#pragma once

#include "QueueFlags.h"
#include "utils/Vector.h"
#include <cstdint>
#ifdef CWDEBUG
#include <iosfwd>
#endif

namespace vulkan {

struct QueueRequest;

// An index for Vectors containing QueueRequest's or QueueReply's.
using QueueRequestIndex = utils::VectorIndex<QueueRequest>;

struct QueueRequest
{
  QueueFlags queue_flags;               // Request a queue with ALL of the flags.
  uint32_t max_number_of_queues = 0;    // Allocate at most this many queues like that. Zero means: as many as supported by the device.
                                        // Zero means: addQueueRequests: as many as supported by the device,
                                        //             combineQueueRequests: the same as the last QueueFlags added (with either addQueueRequests or combineQueueRequests).
  float priority = 0.0;                 // Requested relative priority for those queues.
                                        // Zero means: addQueueRequests: 1.0.
                                        //             combineQueueRequests: the same as the last QueueFlags added (with either addQueueRequests or combineQueueRequests).
  QueueRequestIndex combined_with;      // A previous request that these queues may be combined with if possible.
  uint32_t windows = -1;                // Bit mask with window cookies that this request applies too.

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan
