#include "sys.h"
#include "XML_RPC_Response.h"
#include "utils/AIAlert.h"
#include "utils/print_using.h"
#include "utils/c_escape.h"
#include <iostream>
#include "debug.h"

namespace xmlrpc {

std::array<char const*, 8> data_type_to_str = {
  "std::vector<>",
  "BinaryData",
  "bool",
  "DateTime",
  "double",
  "int32_t",
  "std::string",
  "struct X"
};

std::ostream& operator<<(std::ostream& os, data_type type)
{
  return os << '"' << data_type_to_str[type] << '"';
}

} // namespace xmlrpc

// Default virtual functions.

// Called from XML_RPC_Decoder::start_struct called from Element<element_struct>::start_element.
XML_RPC_Response* XML_RPC_Response::get_struct()
{
  THROW_ALERT("Unexpected <struct>");
}

// Called from XML_RPC_Decoder::start_array called from Element<element_array>::start_element.
XML_RPC_Response* XML_RPC_Response::get_array()
{
  THROW_ALERT("Unexpected <array>");
}

// Called from XML_RPC_Decoder::start_member called from Element<element_name>::end_element.
XML_RPC_Response* XML_RPC_Response::get_member(std::string_view const& name)
{
  // Implement get_member in derived class.
  ASSERT(false);
  return nullptr;
}

// Called from XML_RPC_Decoder::got_characters called from ElementVariable::characters.
void XML_RPC_Response::got_characters(std::string_view const& data)
{
  THROW_ALERT("Unexpected characters [[DATA]] in element", AIArgs("[DATA]", utils::print_using(data, utils::c_escape)));
}

void XML_RPC_Response::got_data()
{
  // Implement got_data in derived class.
  ASSERT(false);
}
