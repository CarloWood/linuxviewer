#pragma once

#include "utils/NodeMemoryPool.h"
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
  virtual ElementDecoder* create_member_decoder(std::string_view const& name);
  virtual ElementDecoder* get_struct_decoder();
  // get_array_decoder never really allocates a new decoder (always returns 'this').
  virtual ElementDecoder* get_array_decoder();

  virtual void got_characters(std::string_view const& data);
  virtual void got_data();
#ifdef CWDEBUG
  virtual void got_member_type(xmlrpc::data_type type, char const* member_name)
  {
    Dout(dc::notice, "=== " << type << " " << member_name << ";");
  }
#endif

  // The opposite of create_member_decoder.
  void destroy_member_decoder();

 protected:
  // All derived classes must be constructed using new (s_pool) Derived(...);
  virtual ~ElementDecoder() { }
  void operator delete(void* ptr) { s_pool.free(ptr); }

 public:
  // Should only be used from xmlrpc::create_member_decoder.
  static utils::NodeMemoryPool s_pool;
};

} // namespace xmlrpc
