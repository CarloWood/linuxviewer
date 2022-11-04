#include "sys.h"
#include "PairTripletIteratorExplode.h"
#include "PartitionTask.h"
#include "ElementPair.h"

namespace vulkan::pipeline::partitions {

PairTripletIteratorExplode::PairTripletIteratorExplode(PartitionTask const& partition_task, Set orig) : m_orig(orig), m_method_A(std::make_shared<method_A_container_t>())
{
  int orig_element_count = m_orig.element_count();
  // Don't try to iterate over a set that has less than 3 elements.
  ASSERT(orig_element_count > 2);
  for (auto element1 = m_orig.begin(); element1 != m_orig.end(); ++element1)
  {
    auto element2 = element1;
    for (++element2; element2 != m_orig.end(); ++element2)
    {
      auto element3 = (orig_element_count > 3) ? element2 : m_orig.end();
      for (++element3;; ++element3)
      {
        bool no_element3 = element3 == m_orig.end();
        Set set23{*element1};
        set23.add(Set{*element2});
        if (!no_element3)
          set23.add(Set{*element3});
        Score score = partition_task.score(set23);
        m_method_A->insert(std::make_pair(score, set23));
        if (no_element3)
          break;
      }
    }
  }
  m_current_A = m_method_A->begin();
}

PairTripletIteratorExplode& PairTripletIteratorExplode::operator++()
{
  if (++m_current_A == m_method_A->end())
    m_orig.clear();
  return *this;
}

Set PairTripletIteratorExplode::operator*() const
{
  return m_current_A->second;
}

// Return amount the score goes down the next iteration.
Score PairTripletIteratorExplode::score_difference() const
{
  // Don't call score_of_next_iteration() on end().
  ASSERT(!is_end());
  ASSERT(m_current_A != m_method_A->end());
  Score score = m_current_A->first;
  method_A_container_t::const_iterator next = m_current_A;
  if (++next == m_method_A->end())
    return {positive_inf};
  Score next_score = next->first;
  return score - next_score;
}

void PairTripletIteratorExplode::print_on(std::ostream& os) const
{
  os << '{';
  os << "  m_orig: " << m_orig << '\n';
  for (method_A_container_t::const_iterator iter = m_method_A->begin(); iter != m_method_A->end(); ++iter)
    os << "  " << *iter << '\n';
  os << "  m_current_A = " << *m_current_A << '\n';
  os << '}';
}

} // namespace vulkan::pipeline::partitions
