#ifndef PARTITION_ITERATOR_H
#define PARTITION_ITERATOR_H

#include "utils/has_print_on.h"
#include <cassert>
#include <memory>

namespace vulkan::pipeline::partitions {
using utils::has_print_on::operator<<;

class PartitionIteratorBase;
class Partition;

class PartitionIterator
{
 private:
  std::unique_ptr<PartitionIteratorBase> m_base;

 public:
  // Create an 'end' iterator.
  PartitionIterator();
  // Create an iterator that runs over the neighboring partitions.
  PartitionIterator(Partition const& orig, std::unique_ptr<PartitionIteratorBase>&& base);
  ~PartitionIterator();

  Partition operator*() const;
  PartitionIterator& operator++();
  friend bool operator!=(PartitionIterator const& lhs, PartitionIterator const& rhs);

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::pipeline::partitions

#endif // PARTITION_ITERATOR_H
