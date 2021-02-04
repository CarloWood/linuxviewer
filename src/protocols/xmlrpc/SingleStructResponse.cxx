#include "sys.h"
#include "SingleStructResponse.h"
#include "utils/AIAlert.h"

namespace xmlrpc {

ElementDecoder* SingleStructResponse::get_struct_decoder()
{
  if (m_saw_struct)
    THROW_FALERT("Unexpected <struct>");
  m_saw_struct = true;
  return this;
}

} // namespace xmlrpc
