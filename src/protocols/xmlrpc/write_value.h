#pragma once

#include "data_types/DateTime.h"
#include "data_types/BinaryData.h"
#include "has_xmlrpc_names.h"
#include <vector>
#include <iostream>
#include <cstdint>
#include <string>

namespace xmlrpc {

void write_value(std::ostream& os, bool val)
{
  os << "<value><boolean>" << val << "</boolean></value>";
}

void write_value(std::ostream& os, int32_t val)
{
  os << "<value><i4>" << val << "</i4></value>";
}

void write_value(std::ostream& os, double val)
{
  os << "<value><double>" << val << "</double></value>";
}

void write_value(std::ostream& os, std::string const& val)
{
  os << "<value><string>" << val << "</string></value>";
}

void write_value(std::ostream& os, DateTime const& val)
{
  os << "<value><dateTime.iso8601>" << val.to_iso8601_string() << "</dateTime.iso8601></value";
}

void write_value(std::ostream& os, BinaryData const& val)
{
  os << "<value><base64>" << val.to_base64_string() << "</base64></value";
}

template<typename T>
void write_value(std::ostream& os, std::vector<T> const& val)
{
  os << "<value><array><data>";
  for (auto&& element : val)
    write_value(os, element);
  os << "</data></array></value>";
}

template<typename S>
struct SerializeMembers
{
  std::ostream& m_output;
  int m_member;

  SerializeMembers(std::ostream& output) : m_output(output), m_member(0) { }

  void write_member_prefix()
  {
    m_output << "<member><name>" << S::s_xmlrpc_names[m_member] << "</name>";
    ++m_member;
  }

  void write_member_postfix()
  {
    m_output << "</member>";
  }

  template<typename T>
  void operator&(T const& val)
  {
    write_member_prefix();
    write_value(m_output, val);
    write_member_postfix();
  }
};

template<typename T>
typename std::enable_if<has_xmlrpc_names_v<T>, void>::type
write_value(std::ostream& os, T const& val)
{
  os << "<value><struct>";
  SerializeMembers<T> sm(os);
  const_cast<T&>(val).serialize(sm, 0);
  os << "</struct></value>";
}

} // namespace xmlrpc
