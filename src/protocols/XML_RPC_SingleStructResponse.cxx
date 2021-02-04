#include "sys.h"
#include "XML_RPC_SingleStructResponse.h"
#include "utils/AIAlert.h"

// static const.
XML_RPC_IgnoreElement XML_RPC_IgnoreElement::s_ignore_element;

XML_RPC_Response* XML_RPC_SingleStructResponse::get_struct()
{
  if (m_saw_struct)
    THROW_FALERT("Unexpected <struct>");
  m_saw_struct = true;
  return this;
}
