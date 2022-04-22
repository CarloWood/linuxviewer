#pragma once

#include "QueueRequest.h"

namespace vulkan {

// The data needed to acquire a requested queue.
//
// This data must uniquely correspond to a single QueueRequest (as requested by the
// user overridden virtual function LogicalDevice::prepare_logical_device) and hence
// a single QueueReply. It can therefore be used to acquire up to QueueReply::number_of_queues()
// queues that support queue_flags by calling LogicalDevice::acquire_queue(QueueRequestKey).
//
// Note that QueueReply objects are hidden; the only available interface is LogicalDevice::acquire_queue
// which will return an empty handle when the combination of flags in queue_flags could not be
// found and less flags should be tried; or throw an exception upon other fatal errors.
//
// Possible errors are: a queue is requested for which the logical device has no support
// at all; or a queue is requested for which no QueueRequest was submitted; or there is
// no one-on-one relationship between such QueueRequest's and the QueueRequestKey that
// was passed to acquire_queue. All of which are logic errors that should terminate
// the program.
//
// However, running out of queues of a given type is not a logical error; the exception
// is the only way to find out that there are no more queues. In this case the exception
// vulkan::OutOfQueues_Exception is thrown.
//
struct QueueRequestKey
{
  using request_cookie_type = vulkan::QueueRequest::cookies_type;

  QueueFlags queue_flags;                                               // An uint32_t bit mask with QueueFlagBits (although only 31 bits seem to be allowed to be used,
                                                                        // see VK_QUEUE_FLAG_BITS_MAX_ENUM).
  request_cookie_type cookie = vulkan::QueueRequest::any_cookie;        // An uint32_t bit mask with user defined meaning.

  // Calculate a "hash" as 64-bit value.
  uint64_t hash() const { return static_cast<uint32_t>(queue_flags) | static_cast<uint64_t>(cookie) << 32; }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan
