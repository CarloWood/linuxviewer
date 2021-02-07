#pragma once

#include "ElementDecoder.h"
#include "create_member_decoder.h"
#include "utils/AIAlert.h"
#include "debug.h"

namespace xmlrpc {

template<typename T>
class SingleStructResponse : public ElementDecoder
{
 private:
  StructDecoder<T> m_struct_decoder;
  bool m_saw_struct;

 protected:
  ElementDecoder* get_struct_decoder() override
  {
    if (m_saw_struct)
      THROW_FALERT("Unexpected <struct>");
    m_saw_struct = true;
    return &m_struct_decoder;
  }

  SingleStructResponse(T& response) : m_struct_decoder(response, 0), m_saw_struct(false) { }
};

} // namespace xmlrpc
