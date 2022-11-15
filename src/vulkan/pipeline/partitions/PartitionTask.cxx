#include "sys.h"
#include "PartitionTask.h"
#include "ElementPair.h"
#include "LogicalDevice.h"
#include <iomanip>

namespace vulkan::pipeline::partitions {

/*
                                             {A}
               {AB}                                                            {A,B}
    {ABC}                {AB,C}          |          {AC,B}                     {A,BC}                          {A,B,C}
{ABCD}  {ABC,D} | {ABD,C}  {AB,CD}  {AB,C,D} | {ACD,B}  {AC,BD}  {AC,B,D} | {AD,BC}  {A,BCD}  {A,BC,D} | {AD,B,C}  {A,BD,C}  {A,B,CD}{A,B,C,D}


{ABCDE} {ABCE,D}  {ABDE,C} {ABE,CD} {ABE,C,D} |{ACDE,B} {ACE,BD} {ACE,B,D} |{ADE,BC} {AE,BCD} {AE,BC,D} |{ADE,B,C} {AE,BD,C} {A,B,CD}{AE,B,C,D}
{ABCD,E}{ABC,DE}  {ABD,CE} {AB,CDE} {AB,CE,D} |{ACD,BE} {AC,BDE} {AC,BE,D} |{AD,BCE} {A,BCDE} {A,BCE,D} |{AD,BE,C} {A,BDE,C} {A,B,CD}{A,BE,C,D}
        {ABC,D,E} {ABD,C,E}{AB,CD,E}{AB,C,DE} |{ACD,B,E}{AC,BD,E}{AC,B,DE} |{AD,BC,E}{A,BCD,E}{A,BC,DE} |{AD,B,CE} {A,BD,CE} {A,B,CD}{A,B,CE,D}
                                    {AB,C,D,E}|                  {AC,B,D,E}|                  {A,BC,D,E}|{AD,B,C,E}{A,BD,C,E}{A,B,CD}{A,B,C,DE}
                                                                                                                                     {A,B,C,D,E}
Table "top_sets = 1" - starting with a partition with a single set.

depth (d)
|
v   1   2   3   4   5 ...     <-- number of sets (sets)
0   1   0   0   0   0 ...  1
1   1   1   0   0   0 ...  2
2   1   3   1   0   0 ...  5
3   1   7   6   1   0 ... 15
4   1  15  25  10   1 ... 52
.           ^
.            \___ number of partitions 'depth' below the top partition that have 'sets' sets.

Table "top_sets = 2" - starting with a partition with two sets.

depth
|
v   1   2   3   4   5 ...     <-- number of sets (sets)
0   0   1   0   0   0 ...  1
1   0   2   1   0   0 ...  3
2   0   4   5   1   0 ... 10
3   0   8  19   9   1 ... 37
4   0  16  65  55  14 ...
.
.

Table "top_sets = 3" - starting with a partition with three sets.

depth
|
v   1   2   3   4   5 ...     <-- number of sets (sets)
0   0   0   1   0   0 ...  1
1   0   0   3   1   0 ...  4
2   0   0   9   7   1 ... 17
3   0   0  27  37  12 ... 76
4   0   0  81 175  97 ...
.
.
*/

PartitionTask::PartitionTask(int8_t number_of_elements, LogicalDevice const* logical_device) :
  m_logical_device(logical_device),
  m_number_of_elements(number_of_elements),
  m_max_number_of_sets(std::min(static_cast<uint32_t>(number_of_elements), logical_device->max_bound_descriptor_sets())),
  m_scores(64 * (number_of_elements - 1) + number_of_elements)
{
}

// Returns a reference into the cache for a given top_sets, depth and sets.
//static
partition_count_t& PartitionTask::number_of_partitions_with_sets(int top_sets, int depth, int sets, table3d_t* table3d)
{
  // The cache is compressed: we don't store the zeroes.
  // That is, the inner array stores the triangle of non-zero values for a given 'Table' (for a given top_sets)
  // and the table for 'top_sets' is shifted top_sets - 1 to the left.
  return (*table3d)[top_sets - 1][depth * (depth + 1) / 2 + sets - top_sets];
}

//static
int PartitionTask::table(int top_sets, int depth, int sets, table3d_t* table3d)
{
  ASSERT(top_sets + depth <= max_number_of_elements);
  if (sets > depth + top_sets || sets < top_sets)
    return 0;
  partition_count_t& te = number_of_partitions_with_sets(top_sets, depth, sets, table3d);
  if (te == 0)
  {
    if (depth == 0)
      te = (sets == top_sets) ? 1 : 0;
    else if (depth + top_sets == sets)
      te = 1;
    else
      te = table(top_sets, depth - 1, sets - 1, table3d) + sets * table(top_sets, depth - 1, sets, table3d);
  }
  return te;
}

// Print the table 'top_sets'.
void PartitionTask::print_table(int top_sets, table3d_t* table3d)
{
  std::cout << "  ";
  for (int8_t sets = 1; sets <= m_max_number_of_sets; ++sets)
  {
    std::cout << std::setw(8) << sets;
  }
  std::cout << '\n';
  for (int8_t depth = 0; depth <= std::min(5, max_number_of_elements - 1 - top_sets); ++depth)   // Use 5 because for larger depth the numbers get way too big.
  {
    partitions::partition_count_t nop = m_logical_device->number_of_partitions(top_sets, depth);
    std::cout << std::setw(2) << depth;
    for (int8_t sets = 1; sets <= m_max_number_of_sets; ++sets)
    {
      int v = table(top_sets, depth, sets, table3d);
      std::cout << std::setw(8) << v;
    }
    std::cout << " = " << nop << '\n';
  }
  std::cout << '\n';
}

void PartitionTask::initialize_set23_to_score()
{
  // Don't call initialize_set23_to_score() twice.
  ASSERT(!m_set23_to_score_initialized);
  m_set23_to_score_initialized = true;
  for (ElementIndex i1 = ibegin(); i1 != iend(); ++i1)
  {
    for (ElementIndex i2 = i1 + 1; i2 != iend(); ++i2)
    {
      ElementPair ep(i1, i2);
      int score_index = ep.score_index();
      Score const score = m_scores[score_index];
      Score zero;
      //Dout(dc::notice, "ep = " << ep << "; score = " << score << "; zero < score = " << std::boolalpha << (zero < score));
      if (zero < score)
        m_set23_to_score[Set(Element{i1}|Element{i2})] = score;
      for (ElementIndex i3 = i2 + 1; i3 != iend(); ++i3)
      {
        ElementPair ep13(i1, i3);
        ElementPair ep23(i2, i3);
        int score_index13 = ep13.score_index();
        int score_index23 = ep23.score_index();
        Score score13 = m_scores[score_index13];
        Score score23 = m_scores[score_index23];
        Score score3 = score;
        score3 += score13;
        score3 += score23;
        if (zero < score3)
          m_set23_to_score[Set(Element{i1}|Element{i2}|Element{i3})] = score3;
      }
    }
  }
}

Partition PartitionTask::random()
{
  Partition top(*this, Set(Element('A')));
  int8_t top_sets = 1;     // The current_root has 1 set.
  int8_t top_elements = 1; // The current_root has 1 element.
  while (top_elements < m_number_of_elements)
  {
    Element const new_element('A' + top_elements);
    int8_t depth = m_number_of_elements - top_elements;
    partition_count_t existing_set = m_logical_device->number_of_partitions(top_sets, depth - 1);
    partition_count_t new_set = m_logical_device->number_of_partitions(top_sets + 1, depth - 1);
    partition_count_t total = top_sets * existing_set + new_set;

    std::uniform_int_distribution<partition_count_t> distr{0, total - 1};
    partition_count_t n = m_random_number.generate(distr);
    if (n >= top_sets * existing_set)
    {
      // Add new element to new set.
      top.add_to(SetIndex{top_sets}, new_element);
      ++top_sets;
    }
    else
    {
      // Add new element to existing set.
      top.add_to(SetIndex{static_cast<int>(n / existing_set)}, new_element);
    }
    ++top_elements;
  }
  return top;
}

#ifdef CWDEBUG
void PartitionTask::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_number_of_elements:" << m_number_of_elements <<
      ", m_set23_to_score:" << m_set23_to_score <<
      ", m_set23_to_score_initialized:" << std::boolalpha << m_set23_to_score_initialized <<
      ", m_scores:" << m_scores;
  os << '}';
}
#endif

} // namespace vulkan::pipeline::partitions
