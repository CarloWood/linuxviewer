#include "sys.h"
#include "UTF8_LLSD_Decoder.h"
#include "debug.h"
#include <cstring>

size_t UTF8_LLSD_Decoder::end_of_msg_finder(char const* new_data, size_t rlen, evio::EndOfMsgFinderResult& UNUSED_ARG(result))
{
  DoutEntering(dc::io|continued_cf, "UTF8_LLSD_Decoder::end_of_msg_finder(\"" << libcwd::buf2str(new_data, rlen) << "\", " << rlen << ", result) = ");

  // ASCII bytes (< 128) do not occur when encoding non-ASCII code points into UTF-8 (all extension bytes are >= 128).
  // It is therefore safe to simply search byte for byte for a '>'.
  char const* right_angle_bracket = static_cast<char const*>(memchr(new_data, '>', rlen));
  size_t found_len = right_angle_bracket ? right_angle_bracket - new_data + 1 : 0;

  Dout(dc::finish, found_len);
  return found_len;
}

void UTF8_LLSD_Decoder::decode(int& allow_deletion_count, evio::MsgBlock&& msg)
{
  DoutEntering(dc::notice, "UTF8_LLSD_Decoder::decode({" << allow_deletion_count << "}, " <<  msg << ")");
}
