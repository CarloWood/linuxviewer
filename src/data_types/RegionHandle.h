#pragma once

class RegionHandle
{
 private:
  int m_x;
  int m_y;

 public:
  RegionHandle() { }
  RegionHandle(int x, int y) : m_x(x), m_y(y) { }

  void set_position(int x, int y) { m_x = x; m_y = y; }
};
