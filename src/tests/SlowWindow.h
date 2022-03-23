#pragma once

#include "Window.h"

class SlowWindow : public Window
{
 public:
  using Window::Window;

 private:
  threadpool::Timer::Interval get_frame_rate_interval() const override
  {
    // Limit the frame rate of this window to 1 frame per second.
    return threadpool::Interval<1000, std::chrono::milliseconds>{};
  }

  bool is_slow() const override
  {
    return true;
  }
};

