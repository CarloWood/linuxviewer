#pragma once

#include "evio/protocol/UTF8_SAX_Decoder.h"
#include "XML_RPC_Response.h"

class XML_RPC_Decoder : public evio::protocol::UTF8_SAX_Decoder
{
 public:
  using ElementBase = evio::protocol::xml::ElementBase;

 private:
  XML_RPC_Response& m_response;
  ElementBase* m_current_element;

 protected:
  void start_document(size_t content_length, std::string version, std::string encoding) final;
  void end_document() final;
  void start_element(index_type element_id) final;
  void end_element(index_type element_id) final;
  void characters(std::string_view const& data) final;

 public:
  XML_RPC_Decoder(XML_RPC_Response& response) : m_response(response) { }
};
