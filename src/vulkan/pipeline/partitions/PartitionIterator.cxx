#include "sys.h"
#include "PartitionIterator.h"
#include "PartitionIteratorBase.h"

namespace vulkan::pipeline::partitions {

PartitionIterator::PartitionIterator(Partition const& orig, std::unique_ptr<PartitionIteratorBase>&& base) :
  m_base(std::move(base))
{
  m_base->kick_start({});
}

PartitionIterator::PartitionIterator()
{
}

PartitionIterator::~PartitionIterator()
{
}

Partition PartitionIterator::operator*() const
{
  Partition p = m_base->original_partition();
  Set moved = m_base->moved_elements();
  assert(!m_base->from_set().undefined());
  assert(p.m_sets.ibegin() <= m_base->from_set());
  assert(m_base->from_set() < p.m_sets.iend());
  p.m_sets[m_base->from_set()].remove(moved);
  assert(!m_base->to_set().undefined());
  assert(p.m_sets.ibegin() <= m_base->to_set());
  assert(m_base->to_set() < p.m_sets.iend());
  p.m_sets[m_base->to_set()].add(moved);
  p.sort();
  return p;
}

PartitionIterator& PartitionIterator::operator++()
{
  m_base->increment();
  return *this;
}

bool operator!=(PartitionIterator const& lhs, PartitionIterator const& rhs)
{
  if (lhs.m_base && rhs.m_base)
    return lhs.m_base->unequal(*rhs.m_base);
  bool lhs_is_end = !lhs.m_base || lhs.m_base->is_end();
  bool rhs_is_end = !rhs.m_base || rhs.m_base->is_end();
  return lhs_is_end != rhs_is_end;
}

void PartitionIterator::print_on(std::ostream& os) const
{
  if (m_base)
    os << "{m_base:@" << *m_base << "}";
  else
    os << "{m_base:nullptr}";
}

} // namespace vulkan::pipeline::partitions
