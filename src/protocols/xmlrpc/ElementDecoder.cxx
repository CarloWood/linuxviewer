#include "sys.h"
#include "IgnoreElement.h"
#include "ArrayOfStructDecoder.h"
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

// Default virtual functions.

// Called from XML_RPC_Decoder::start_struct called from Element<element_struct>::start_element.
ElementDecoder* ElementDecoder::get_struct_decoder()
{
  THROW_ALERT("Unexpected <struct>");
}

// Called from XML_RPC_Decoder::start_array called from Element<element_array>::start_element.
ElementDecoder* ElementDecoder::get_array_decoder()
{
  THROW_ALERT("Unexpected <array>");
}

// Called from XML_RPC_Decoder::start_member called from Element<element_name>::end_element.
ElementDecoder* ElementDecoder::create_member_decoder(std::string_view const& name)
{
  // Implement get_member in derived class.
  ASSERT(false);
  return nullptr;
}

void ElementDecoder::destroy_member_decoder()
{
  // To be implemented.
  if (this != &IgnoreElement::s_ignore_element)
    delete this;
}

// Called from XML_RPC_Decoder::got_characters called from ElementVariable::characters.
void ElementDecoder::got_characters(std::string_view const& data)
{
  THROW_ALERT("Unexpected characters [[DATA]] in element", AIArgs("[DATA]", utils::print_using(data, utils::c_escape)));
}

void ElementDecoder::got_data()
{
  // Implement got_data in derived class.
  ASSERT(false);
}

// ArrayOfStructDecoder is the largest.
struct Dummy { enum members { one_ }; };
static constexpr size_t largest_size = sizeof(ArrayOfStructDecoder<Dummy>);
utils::NodeMemoryPool ElementDecoder::s_pool(8, largest_size);

} // namespace xmlrpc
