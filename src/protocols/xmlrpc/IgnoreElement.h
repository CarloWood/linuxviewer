#pragma once

#include "ElementDecoder.h"
#include <libcwd/buf2str.h>
#include "debug.h"

namespace xmlrpc {

class IgnoreElement : public ElementDecoder
{
  ElementDecoder* get_struct_decoder() override { return this; }
  ElementDecoder* get_array_decoder() override { return this; }
  ElementDecoder* get_member_decoder(std::string_view const&) override { return this; }

#ifdef CWDEBUG
  void got_member_type(xmlrpc::data_type type, char const* struct_name) override { Dout(dc::notice, "got_member_type(" << type << ", \"" << struct_name << "\") ignored"); }
#endif
  void got_characters(std::string_view const& data) override { Dout(dc::notice, "got_characters(" << libcwd::buf2str(data.data(), data.size()) << ") ignored"); }
  void got_data() override { Dout(dc::notice, "got_data() ignored"); }

 public:
  static IgnoreElement s_ignore_element;
};

} // namespace xmlrpc
