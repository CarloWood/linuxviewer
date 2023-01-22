#pragma once

#include "Partition.h"
#include "utils/RandomNumber.h"
#include <map>
#include <vector>
#include <array>
#ifdef USE_BRUTE_FORCE_ITERATOR         // Normally not defined.
#include "PartitionIteratorBruteForce.h"
#endif

namespace vulkan {
class LogicalDevice;
} // namespace vulkan

namespace vulkan::pipeline::partitions {

class PartitionTask
{
 private:
  LogicalDevice const* m_logical_device;        // The device that this is being used for: it determines the max_number_of_sets.
  int8_t m_number_of_elements;                  // The number of elements that we need to partition.
  int8_t m_max_number_of_sets;                  // The maximum number of sets that will be used by this task.
  utils::RandomNumber m_random_number{1};       // Random number generator for generating random partitions.
  std::map<Set, Score> m_set23_to_score;        // Initialized by initialize_set23_to_score.
  bool m_set23_to_score_initialized{false};     // Set to true when m_set23_to_score is initialized.
  std::vector<Score> m_scores;

 public:
  PartitionTask(int8_t number_of_elements, LogicalDevice const* logical_device);

  static partition_count_t& number_of_partitions_with_sets(int top_sets, int depth, int sets, table3d_t* table3d);
  static int table(int top_sets, int depth, int sets, table3d_t* table3d);
  void print_table(int top_sets, table3d_t* table3d);

  Score score(Set set23) const
  {
    auto set23_iter = m_set23_to_score.find(set23);
    if (set23_iter == m_set23_to_score.end())
      return {};
    return set23_iter->second;
  }

  Partition random();

  int8_t number_of_elements() const
  {
    return m_number_of_elements;
  }

  int8_t max_number_of_sets() const
  {
    return m_max_number_of_sets;
  }

  ElementIndex ibegin() const
  {
    return ElementIndexPOD{0};
  }

  ElementIndex iend() const
  {
    return ElementIndexPOD{m_number_of_elements};
  }

#ifdef USE_BRUTE_FORCE_ITERATOR
  PartitionIteratorBruteForce bbegin(PartitionTask const& partition_task)
  {
    return {partition_task};
  }

  PartitionIteratorBruteForce bend()
  {
    return {};
  }
#endif

  void initialize_set23_to_score();
  void initialize_scores();

  void set_score(int score_index, Score score)
  {
    m_scores[score_index] = score;
  }

  Score const& score(int score_index) const
  {
    return m_scores[score_index];
  }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::pipeline::partitions
