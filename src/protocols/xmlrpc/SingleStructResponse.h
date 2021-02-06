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
  bool m_saw_struct;

 protected:
  SingleStructResponse() : m_saw_struct(false) { }

  ElementDecoder* get_struct_decoder() override;
};

template<typename T>
ElementDecoder* SingleStructResponse<T>::get_struct_decoder()
{
  if (m_saw_struct)
    THROW_FALERT("Unexpected <struct>");
  m_saw_struct = true;
  return create_member_decoder(*static_cast<T*>(this), 0);
}

} // namespace xmlrpc
