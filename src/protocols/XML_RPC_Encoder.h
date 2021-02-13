#pragma once

#include <iosfwd>

namespace xmlrpc {
class Request;
}// namespace xmlrpc

class XML_RPC_Encoder
{
 private:
  std::ostream& m_output;

 public:
  XML_RPC_Encoder(std::ostream& output) : m_output(output) { }

  friend XML_RPC_Encoder& operator<<(XML_RPC_Encoder& encoder, xmlrpc::Request const& request);
};
