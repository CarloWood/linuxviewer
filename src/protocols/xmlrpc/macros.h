#pragma once

#include "debug.h"

#define XMLRPC_DECLARE_ENUMERATOR(type, el, ...) member_##el,
#define XMLRPC_DECLARE_MEMBER(type, el, ...) type m_##el;
#define XMLRPC_DECLARE_PARAMETERS(type, el, ...) type p_##el,
#define XMLRPC_INITIALIZER_LIST(type, el, ...) , m_##el(p_##el)
#define XMLRPC_INITIALIZER_LIST_FROM_PARAMS(type, el, ...) , m_##el(params.m_##el)
#define XMLRPC_CASE_RETURN_MEMBER_DECODER(type, el, ...) case member_##el: return xmlrpc::create_member_decoder(m_##el);

#define XMLRPC_BOOST_SERIALIZE(type, el, ...) ar & m_##el;
#define XMLRPC_WRITE_TO_OS(type, el, ...) os << prefix << "m_" #el ": " << m_##el; prefix = ", ";

#define XMLRPC_PLUS_ONE(type, el, ...) + 1
#define XMLRPC_NUMBER_OF_MEMBERS(Class) 0 xmlrpc_##Class##_FOREACH_MEMBER(XMLRPC_PLUS_ONE)

#define XMLRPC_DECLARE_NAME_3_ARGS(type, el, name) name,
#define XMLRPC_DECLARE_NAME_2_ARGS(type, el) XMLRPC_DECLARE_NAME_3_ARGS(type, el, #el)
#define XMLRPC_GET_4TH_ARG(arg1, arg2, arg3, arg4, ...) arg4
#define XMLRPC_DECLARE_NAME_MACRO_CHOOSER(...) XMLRPC_GET_4TH_ARG(__VA_ARGS__, XMLRPC_DECLARE_NAME_3_ARGS, XMLRPC_DECLARE_NAME_2_ARGS, )
#define XMLRPC_DECLARE_NAME(...) XMLRPC_DECLARE_NAME_MACRO_CHOOSER(__VA_ARGS__)(__VA_ARGS__)

namespace xmlrpc {
class ElementDecoder;
} // namespace xmlrpc
