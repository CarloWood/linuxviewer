#include "sys.h"
#include "TimerData.h"
#include <iostream>
#include "debug.h"

namespace vk_utils {

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

void TimerData::print_on(std::ostream& os) const
{
  os << "{m_moving_average_ms:" << m_moving_average_ms << ", m_moving_average_FPS:" << m_moving_average_FPS <<'}';
}

} // namespace vk_utils