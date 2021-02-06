#pragma once

#include <string_view>
#include <array>
#include <iosfwd>
#include "debug.h"

namespace xmlrpc {

enum data_type
{
  data_type_array,
  data_type_base64,
  data_type_boolean,
  data_type_datetime,
  data_type_double,
  data_type_int,
  data_type_string,
  data_type_struct
};

extern std::array<char const*, 8> data_type_to_str;
extern std::ostream& operator<<(std::ostream& os, data_type type);

class ElementDecoder
{
 public:
  virtual ElementDecoder* get_struct_decoder();
  virtual ElementDecoder* get_array_decoder();
  virtual ElementDecoder* get_member_decoder(std::string_view const& name);
  virtual void got_characters(std::string_view const& data);
  virtual void got_data();

#ifdef CWDEBUG
  virtual void got_member_type(xmlrpc::data_type type, char const* member_name)
  {
    Dout(dc::notice, "=== " << type << " " << member_name << ";");
  }
#endif
};

} // namespace xmlrpc
