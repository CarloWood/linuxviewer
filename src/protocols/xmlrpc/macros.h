#pragma once

#include "debug.h"

#define XMLRPC_DECLARE_ENUMERATOR(type, el) member_##el,
#define XMLRPC_DECLARE_MEMBER(type, el) type m_##el;
#define XMLRPC_DECLARE_MEMBER_NAME(type, el) #el,
#define XMLRPC_CASE_RETURN(type, el) case member_##el: return #el;
#define XMLRPC_CASE_RETURN_MEMBER_DECODER(type, el) case member_##el: return xmlrpc::create_member_decoder(m_##el);

namespace xmlrpc {
class ElementDecoder;
} // namespace xmlrpc
