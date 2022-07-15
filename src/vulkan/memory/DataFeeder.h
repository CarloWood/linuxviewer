#pragma once

#include <cstdint>

namespace vulkan {

// Feeds the caller with data in chunks of one or more "chunks".
//
// To get the data call chunk_size() and chunk_count() once,
// then call next_batch() and get_chunks() in a loop:
//
// uint32_t size = df.chunk_size();
// int chunk_count = df.chunk_count();
// int next_batch;
// for (int chunks = 0; chunks < chunk_count; chunks += next_batch)
// {
//   next_batch = df.next_batch();
//   // Prepare to read next_batch * chunk_size bytes.
//   ptr = ...
//   df.get_chunks(ptr);
// }
//
class DataFeeder
{
 public:
  virtual ~DataFeeder() = default;

  // The size of one chunk in bytes; must be a multiple of the required alignment.
  virtual uint32_t chunk_size() const = 0;

  // The total number of chunks.
  virtual int chunk_count() const = 0;

  // The value returned is the number of chunks that will be initialized by the next call to get_chunks.
  // Once the sum of all returned values reaches chunk_count(), no more calls to next_batch or get_chunks
  // must be made.
  virtual int next_batch() = 0;

  // Fills in N chunks, where N is the value that was returned by the last call to next_batch().
  virtual void get_chunks(unsigned char* chunk_ptr) = 0;
};

} // namespace vulkan
