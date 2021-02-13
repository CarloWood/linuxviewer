#include "sys.h"
#include "LoginToSimulator.h"
#include "protocols/xmlrpc/write_value.h"
#include <iostream>

template<typename T>
std::ostream& operator<<(std::ostream& os, std::vector<T> const& v)
{
  char const* prefix = "{";
  for (auto&& element : v)
  {
    os << prefix << element;
    prefix = ", ";
  }
  return os << '}';
}

namespace xmlrpc {

constexpr int LoginToSimulator::s_number_of_members;
constexpr std::array<char const*, LoginToSimulator::s_number_of_members> LoginToSimulator::s_xmlrpc_names;

void LoginToSimulator::print_on(std::ostream& os) const
{
  char const* prefix = "";
  xmlrpc_LoginToSimulator_FOREACH_MEMBER(XMLRPC_WRITE_TO_OS)
}

void LoginToSimulator::write_param(std::ostream& output) const
{
  output << "<param>";
  write_value(output, *this);
  output << "</param>";
}

} // namespace xmlrpc
