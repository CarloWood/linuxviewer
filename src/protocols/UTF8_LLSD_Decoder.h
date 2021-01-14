#pragma once

#include "evio/protocol/DecoderStream.h"

// This decoder deserializes LLSD.
//
// It only supports UTF-8.
//
class UTF8_LLSD_Decoder : public evio::protocol::Decoder
{
 protected:
  size_t end_of_msg_finder(char const* new_data, size_t rlen, evio::EndOfMsgFinderResult& result) final;
  void decode(int& allow_deletion_count, evio::MsgBlock&& msg) override;
};
