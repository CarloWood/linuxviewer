#pragma once

#include "evio/protocol/DecoderStream.h"

class LlsdDecoder : public evio::protocol::Decoder
{
 private:
  bool m_last_char_is_angle_open_bracket;
  bool m_inside_closing_tag;

 public:
  LlsdDecoder() : m_last_char_is_angle_open_bracket(false), m_inside_closing_tag(false) { }

 protected:
  size_t end_of_msg_finder(char const* new_data, size_t rlen, evio::EndOfMsgFinderResult& result) override;
  void decode(int& allow_deletion_count, evio::MsgBlock&& msg) override;
};
