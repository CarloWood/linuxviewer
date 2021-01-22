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

 private:
  std::array<variant_type, data_type::s_number_of_members> m_data;

  // d[member] = 3.1;
  variant_type& operator[](members member) { return m_data[member]; }
  // double d = std::get<double>(d[member]);
  variant_type const& operator[](members member) const { return m_data[member]; }

 private:
  // Implementation.
  variant_type& operator[](int index) final { ASSERT(0 <= index && index < data_type::s_number_of_members); return m_data[index]; }
};
