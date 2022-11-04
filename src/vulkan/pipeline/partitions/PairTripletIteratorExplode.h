#pragma once

#include "Set.h"
#include "Score.h"
#include <map>
#include <mutex>
#include <memory>

// {ABCF}
//
// {AB}
// {AC}
// {AF}
// {BC}
// {BF}
// {CF}
// {ABC}
// {ABF}
// {ACF}
// {BCF}

namespace vulkan::pipeline::partitions {

class PairTripletIteratorExplode
{
 private:
  Set m_orig;
  using method_A_container_t = std::multimap<Score, Set, std::greater<Score>>;
  std::shared_ptr<method_A_container_t> m_method_A;
  method_A_container_t::const_iterator m_current_A;

 public:
  // Create the begin iterator.
  PairTripletIteratorExplode(PartitionTask const& partition_task, Set orig);

  static void initialize();

  PairTripletIteratorExplode& operator++();

  void reset()
  {
    m_current_A = m_method_A->begin();
  }

  bool is_end() const
  {
    return m_current_A == m_method_A->end();
  }

  Set operator*() const;

  Score score_difference() const;

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif // CWDEBUG
};

} // namespace vulkan::pipeline::partitions
