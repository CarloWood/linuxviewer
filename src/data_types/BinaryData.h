#pragma once

#include <vector>
#include <string>
#include <cstddef>
#include <iosfwd>

class BinaryData
{
 public:
  using data_type = std::vector<char>;  // I'd like to use std::byte, but boost::archive doesn't like that very much :(.

 private:
  data_type m_data;

 public:
  BinaryData() = default;

  void assign_from_binary(std::string_view const& data) { m_data.assign(data.begin(), data.end()); }
  void assign_from_base64(std::string_view const& data);

  std::string to_base64_string() const;
  std::string to_hexadecimal_string() const;
  std::string to_xml_escaped_string() const;
  std::string to_c_escaped_string() const;

  std::string to_string() const { return {m_data.begin(), m_data.end()}; }

  void print_on(std::ostream& os) const;        // Print as escaped string with quotes around it.
};
