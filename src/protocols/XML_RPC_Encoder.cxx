#include "sys.h"
#include "XML_RPC_Encoder.h"
#include "xmlrpc/Request.h"
#include "debug.h"

XML_RPC_Encoder& operator<<(XML_RPC_Encoder& encoder, xmlrpc::Request const& request)
{
#if 0
  // Write HTTP header.
  encoder.m_output <<
    "POST / HTTP/1.1\r\n"
    "Host: " << << "\r\n"
    "Accept: */*\r\n"
    "Connection: close\r\n"
    "Content-Type: text/xml\r\n"
    "Content-Length: 2072\r\n"
    "\r\n"
#endif

  // Write XML RPC header.
  encoder.m_output <<
    "<?xml version=\"1.0\"?>"
    "<methodcall>"
      "<methodName>" << request.method_name() << "</methodName>"
      "<params>";

  for(auto&& param : request)
    param->write_param(encoder.m_output);

  // Write XML RPC trailer.
  encoder.m_output <<
      "</params>"
    "</methodCall>";

  return encoder;
}
