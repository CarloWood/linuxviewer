#pragma once

#include "XML_RPC_Response.h"
#include <array>
#include <variant>

template<typename T>
class XML_RPC_Data : public XML_RPC_Response
{
 public:
  using data_type = T;
  using members = typename data_type::members;
  using variant_type = std::variant<bool, double, int32_t, std::string>;

 private:
  std::array<variant_type, data_type::s_number_of_members> m_data;

  // d[member] = 3.1;
  variant_type& operator[](members member) { return m_data[member]; }
  // double d = std::get<double>(d[member]);
  variant_type const& operator[](members member) const { return m_data[member]; }
};
