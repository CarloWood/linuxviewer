#pragma once

#include "evio/protocol/DecoderStream.h"

class LlsdDecoder : public evio::protocol::DecoderStream
{
 private:
  bool m_last_char_is_angle_open_bracket;
  bool m_inside_closing_tag;

 public:
  LlsdDecoder() : m_last_char_is_angle_open_bracket(false), m_inside_closing_tag(false) { }

 protected:
  size_t end_of_msg_finder_stream(char const* CWDEBUG_ONLY(new_data), size_t rlen) final
  {
    DoutEntering(dc::io, "LlsdDecoder::end_of_msg_finder_stream(\"" << libcwd::buf2str(new_data, rlen) << "\", " << rlen << ") = " << rlen);
    return rlen;
  }
  void decode(int& allow_deletion_count, size_t msg_len) override;
};
