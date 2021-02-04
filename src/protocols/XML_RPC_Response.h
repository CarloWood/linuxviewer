#pragma once

#include <string_view>
#include <array>
#include <iosfwd>
#ifdef CWDEBUG
#include <string>
#endif

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

} // namespace xmlrpc

class XML_RPC_Response
{
 protected:
#ifdef CWDEBUG
  char const* m_struct_name;
#endif

 public:
  virtual XML_RPC_Response* get_struct();
  virtual XML_RPC_Response* get_array();
  virtual XML_RPC_Response* get_member(std::string_view const& name);
  virtual void got_characters(std::string_view const& data);
  virtual void got_data();

#ifdef CWDEBUG
  virtual void got_member_type(xmlrpc::data_type type, char const* struct_name) { }
  virtual char const* get_struct_name() const { return m_struct_name; }
#endif
};
