#include "sys.h"
#include "TimerData.h"
#include <iostream>
#include "debug.h"

namespace vk_utils {

// Initially the state is:
//
// index: 0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16 = s_history_size
//     [ ??, ??, ??, ??, ??, ??, ??, ??, ??, ??, ??, ??, ??, ??, ??, ??, ?? ]   <-- array size is s_history_size + 1.
//        ^   ^
//        |   |
//        |  m_first_index
// m_current_index
//
// After one call to update (at now = 42), we have:
//
// index: 0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16
//     [ ??, 42, ??, ??, ??, ??, ??, ??, ??, ??, ??, ??, ??, ??, ??, ??, ?? ]
//            ^
//            |\
//            |  m_first_index
//     m_current_index
//
// After the second call (at now = 43) we have:
//
// index: 0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16
//     [ ??, 42, 43, ??, ??, ??, ??, ??, ??, ??, ??, ??, ??, ??, ??, ??, ?? ]
//            ^   ^
//            |   |                                                             During this call, intervals = 1.
//   m_first_time |
//         m_current_index
//
// And so on until after the 16th call to update():
//
// index: 0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16
//     [ ??, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57 ]
//            ^                                                           ^
//            |                                                           |     During this call, intervals = 15.
//   m_first_time                                                         |
//                                                                 m_current_index
//
// After the 17th call:
//
// index: 0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16
//     [ ??, 58, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57 ]
//            ^   ^
//            |   |                                                             During this call, intervals = 0 then set to 15 and m_first_time is incremented to 2.
//            |  m_first_time
//     m_current_index
//
// And so on until after the 31th call to update():
//
// index: 0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16
//     [ ??, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 57 ]
//                                                                    ^   ^
//                                                                    |   |     During this call, intervals = 0 then set to 15 and m_first_time is incremented to 16.
//                                                                    |  m_first_time
//                                                             m_current_index
// After the 32th call:
//
// index: 0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16
//     [ ??, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73 ]
//            ^                                                           ^
//            |                                                           |     During this call, intervals = 0 then set to 15 and m_first_time is set to 1.
//        m_first_time                                                    |
//                                                                 m_current_index
//
// After 33th call the state becomes the same as after the 17th call.

void TimerData::update()
{
  bool first_time = m_current_index == 0;

  // Store last time.
  auto previous_time = m_time_history[m_current_index];

  // Increment the index.
  m_current_index = (m_current_index % s_history_size) + 1;

  // Store current time.
  m_time_history[m_current_index] = std::chrono::high_resolution_clock::now();

  if (!first_time)
  {
    // Number of intervals. This is 1 during the second call because m_current_index starts at 0, and m_first_index starts at 1.
    int intervals = (m_current_index - m_first_index + s_history_size) % s_history_size;
    // Update first measurements.
    if (intervals == 0)      // Array full? Then throw away the oldest measurement.
    {
      m_first_index = (m_first_index % s_history_size) + 1;
      intervals = s_history_size - 1;
    }
    // Cache averages.
    m_moving_average_ms = std::chrono::duration<float, std::milli>(m_time_history[m_current_index] - m_time_history[m_first_index]).count() / intervals;
    m_moving_average_FPS = 1000.f / m_moving_average_ms;
    // Cache delta time.
    m_delta_ms = std::chrono::duration<float, std::milli>(m_time_history[m_current_index] - previous_time).count();
  }
}

std::array<float, TimerData::s_history_size - 1> TimerData::get_FPS_histogram() const
{
  std::array<float, s_history_size - 1> result;
  int intervals = m_current_index ? (m_current_index - m_first_index + s_history_size) % s_history_size : 0;
  int index = m_current_index;
  int n = s_history_size - 1;
  while (n >= s_history_size - intervals)
  {
    int next_index = (index - 2 + s_history_size) % s_history_size + 1;
    result[--n] = 1000.f / std::chrono::duration<float, std::milli>(m_time_history[index] - m_time_history[next_index]).count();
    index = next_index;
  }
  while (n > 0)
  {
    result[--n] = 0.f;
  }
  return result;
}

std::array<float, TimerData::s_history_size - 1> TimerData::get_delta_ms_histogram() const
{
  std::array<float, s_history_size - 1> result;
  int intervals = m_current_index ? (m_current_index - m_first_index + s_history_size) % s_history_size : 0;
  int index = m_current_index;
  int n = s_history_size - 1;
  while (n >= s_history_size - intervals)
  {
    int next_index = (index - 2 + s_history_size) % s_history_size + 1;
    result[--n] = std::chrono::duration<float, std::milli>(m_time_history[index] - m_time_history[next_index]).count();
    index = next_index;
  }
  while (n > 0)
  {
    result[--n] = 0.f;
  }
  return result;
}

void TimerData::print_on(std::ostream& os) const
{
  os << "{m_moving_average_ms:" << m_moving_average_ms << ", m_moving_average_FPS:" << m_moving_average_FPS <<'}';
}

} // namespace vk_utils
