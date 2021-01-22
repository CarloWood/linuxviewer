#pragma once

#include <variant>

class XML_RPC_Response
{
 public:
  using variant_type = std::variant<bool, double, int32_t, std::string>;

 public:
  variant_type const& operator[](int index) const { return const_cast<XML_RPC_Response*>(this)->operator[](index); }
  virtual variant_type& operator[](int index) = 0;
};
