#pragma once

#include "evio/protocol/DecoderStream.h"
#include "xml/Reader.h"
#include <functional>

class GridInfo;

class GridInfoDecoder : public evio::protocol::DecoderStream
{
 private:
  GridInfo& m_grid_info;
  std::function<int()> m_get_message_length;  // m_get_message_length() should return the total size of the input by the time end_of_msg_finder is called.
  int m_message_length;
  int m_total_len;
  Sink& m_next_decoder;

 public:
  GridInfoDecoder(GridInfo& grid_info, std::function<int()> get_message_length, Sink& next_decoder) :
    m_grid_info(grid_info), m_get_message_length(get_message_length), m_message_length(-1), m_total_len(0), m_next_decoder(next_decoder) { }

 protected:
  size_t end_of_msg_finder(char const* new_data, size_t rlen, evio::EndOfMsgFinderResult& result) override;
  void decode(int& allow_deletion_count) override;
};
