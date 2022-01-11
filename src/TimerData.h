#include <array>
#include <chrono>
#include <iosfwd>

namespace lv_utils {

// TimerData
//
// Class for time and FPS measurements.
//
class TimerData
{
 public:
  static constexpr int history_size = 16;

 private:
  std::array<std::chrono::time_point<std::chrono::high_resolution_clock>, history_size> m_time_history;
  int m_first_index;
  int m_current_index;
  float m_moving_average_ms = {};
  float m_moving_average_FPS = {};

 public:
  float get_moving_average_ms() const { return m_moving_average_ms; }
  float get_moving_average_FPS() const { return m_moving_average_FPS; }

  void update();

  TimerData() : m_first_index(0), m_current_index(-1) { }

  void print_on(std::ostream& os) const;
  friend std::ostream& operator<<(std::ostream& os, TimerData const& data) { data.print_on(os); return os; }
};

} // namespace lv_utils
