#pragma once

#include "evio/protocol/DecoderStream.h"
#include "xml/Reader.h"
#include <functional>

class GridInfo;

class GridInfoDecoder : public evio::protocol::DecoderStream
{
 private:
  GridInfo& m_grid_info;

 public:
  GridInfoDecoder(GridInfo& grid_info) : m_grid_info(grid_info) { }

 protected:
  void decode(int& allow_deletion_count, size_t msg_len) override;
};
