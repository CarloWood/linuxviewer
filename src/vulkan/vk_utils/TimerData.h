#include <array>
#include <chrono>
#include <iosfwd>

namespace vk_utils {

// TimerData
//
// Class for time and FPS measurements.
//
class TimerData
{
 public:
  static constexpr int s_history_size = 16;

 private:
  int m_first_index;
  int m_current_index;
  std::array<std::chrono::time_point<std::chrono::high_resolution_clock>, s_history_size + 1> m_time_history;     // Index [1...s_history_size] are used.
  float m_moving_average_ms = {};       // Delta time averaged over the last s_history_size frames.
  float m_moving_average_FPS = {};      // Frames Per Second, averaged over the last s_history_size frames.
  float m_delta_ms = {};                // Delta time since last frame, in ms.

 public:
  float get_moving_average_ms() const { return m_moving_average_ms; }
  float get_moving_average_FPS() const { return m_moving_average_FPS; }
  float get_delta_ms() const { return m_delta_ms; }

  void update();

  // Only the very first time m_current_index will be equal to zero. After that it falls in the range [0, s_history_size] inclusive.
  TimerData() : m_first_index(1), m_current_index(0) { }

  void print_on(std::ostream& os) const;
  friend std::ostream& operator<<(std::ostream& os, TimerData const& data) { data.print_on(os); return os; }
};

} // namespace vk_utils
