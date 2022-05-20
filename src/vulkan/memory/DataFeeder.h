#pragma once

#include <cstdint>

namespace vulkan {

// Feeds the caller with data in chunks of one or more "fragments".
//
// To get the data call fragment_size() and fragment_count() once,
// then call next_batch() and get_fragments() in a loop:
//
// uint32_t size = df.fragment_size();
// int fragment_count = df.fragment_count();
// int next_batch;
// for (int fragments = 0; fragments < fragment_count; fragments += next_batch)
// {
//   next_batch = df.next_batch();
//   // Prepare to read next_batch * fragment_size bytes.
//   ptr = ...
//   df.get_fragments(ptr);
// }
//
class DataFeeder
{
 public:
  virtual ~DataFeeder() = default;

  // The size of one fragment in bytes; must be a multiple of the required alignment.
  virtual uint32_t fragment_size() const = 0;

  // The total number of fragments.
  virtual int fragment_count() const = 0;

  // The value returned is the number of fragments that will be initialized by the next call to get_fragments.
  // Once the sum of all returned values reaches fragment_count(), no more calls to next_batch or get_fragments
  // must be made.
  virtual int next_batch() = 0;

  // Fills in N fragments, where N is the value that was returned by the last call to next_batch().
  virtual void get_fragments(unsigned char* fragment_ptr) = 0;
};

} // namespace vulkan
