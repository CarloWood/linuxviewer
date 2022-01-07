#include "sys.h"
#include "TimerData.h"
#include <iostream>
#include "debug.h"

namespace lv_utils {

void TimerData::update()
{
  bool first_time = m_current_index == -1;

  // Increment the index.
  m_current_index = (m_current_index + 1) % history_size;

  // Store current time.
  m_time_history[m_current_index] = std::chrono::high_resolution_clock::now();

  if (!first_time)
  {
    // Number of intervals. This is 1 during the second call because m_current_index starts at -1, and m_first_index starts at 0.
    int intervals = (m_current_index - m_first_index + history_size) % history_size;
    // Update first measurements.
    if (intervals == 0)      // Array full? Then throw away the oldest measurement.
    {
      m_first_index = (m_first_index + 1) % history_size;
      intervals = history_size - 1;
    }
    // Cache averages.
    m_moving_average_ms = std::chrono::duration<float, std::milli>(m_time_history[m_current_index] - m_time_history[m_first_index]).count() / intervals;
    m_moving_average_FPS = 1000.f / m_moving_average_ms;
  }
}

void TimerData::print_on(std::ostream& os) const
{
  os << "{m_moving_average_ms:" << m_moving_average_ms << ", m_moving_average_FPS:" << m_moving_average_FPS <<'}';
}

} // namespace lv_utils
