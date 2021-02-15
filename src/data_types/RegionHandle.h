#pragma once

#include <string_view>
#ifdef CWDEBUG
#include <iosfwd>
#endif

class RegionHandle
{
 private:
  int m_x;
  int m_y;

 public:
  RegionHandle() { }
  RegionHandle(int x, int y) : m_x(x), m_y(y) { }

  void set_position(int x, int y) { m_x = x; m_y = y; }

  void assign_from_xmlrpc_string(std::string_view const& data);

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};
