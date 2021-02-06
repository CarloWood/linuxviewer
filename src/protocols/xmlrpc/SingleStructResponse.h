#pragma once

#include "ElementDecoder.h"
#include "debug.h"

namespace xmlrpc {

class SingleStructResponse : public ElementDecoder
{
 private:
  bool m_saw_struct;

 protected:
  SingleStructResponse() : m_saw_struct(false) { }

  ElementDecoder* get_struct_decoder() override;
};

} // namespace xmlrpc
