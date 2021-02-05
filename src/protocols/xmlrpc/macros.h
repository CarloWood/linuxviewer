#pragma once

#include "debug.h"

#define XMLRPC_DECLARE_ENUMERATOR(flags, type, el) member_##el,
#define XMLRPC_DECLARE_MEMBER(flags, type, el) type m_##el;
#define XMLRPC_DECLARE_MEMBER_NAME(flags, type, el) #el,
#define XMLRPC_CASE_RETURN(flags, type, el) case member_##el: return #el;
#define XMLRPC_CASE_RETURN_MEMBER_DECODER(flags, type, el) case member_##el: return xmlrpc::create_member_decoder(m_##el, flags COMMA_CWDEBUG_ONLY(#el));

namespace xmlrpc {
class ElementDecoder;
} // namespace xmlrpc
