#pragma once

#include "Partition.h"
#include "utils/MultiLoop.h"
#include <array>

namespace vulkan::pipeline::partitions {

class PartitionTask;

class PartitionIteratorBruteForce
{
 private:
  MultiLoop m_multiloop;
  std::vector<int> m_loop_value_count;       // The number of loop counter with a value equal to the index.
  PartitionTask const* m_partition_task{};

 public:
  // Create an end iterator.
  PartitionIteratorBruteForce() : m_multiloop(1, 1) { }
  // Create the begin iterator.
  PartitionIteratorBruteForce(PartitionTask const& partition_task);

  PartitionIteratorBruteForce& operator++();

  bool is_end() const
  {
    return m_multiloop.finished();
  }

  Partition operator*() const;
};

} // namespace vulkan::pipeline::partitions
