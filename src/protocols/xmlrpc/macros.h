#pragma once

#include "debug.h"

#define XMLRPC_DECLARE_ENUMERATOR(type, el, ...) member_##el,
#define XMLRPC_DECLARE_MEMBER(type, el, ...) type m_##el;
#define XMLRPC_DECLARE_PARAMETER(type, el, ...) type p_##el,
#define XMLRPC_INITIALIZER_LIST(type, el, ...) m_##el(p_##el),
#define XMLRPC_INITIALIZER_LIST_FROM_PARAMS(type, el, ...) , m_##el(params.m_##el)

// Generate code inside a switch on members enumerator to call create_member_decoder with the right member variable.
#define XMLRPC_CASE_RETURN_MEMBER_DECODER(type, el, ...) case member_##el: return xmlrpc::create_member_decoder(m_##el);

// Generate code inside the serialize template function.
#define XMLRPC_BOOST_SERIALIZE(type, el, ...) ar & m_##el;

// Generate code inside the print_to debug function.
#define XMLRPC_WRITE_TO_OS(type, el, ...) os << prefix << "m_" #el ": " << m_##el; prefix = ", ";

// Count the number of 'X' macros used in the FOREACH_MEMBER declaration.
#define XMLRPC_PLUS_ONE(type, el, ...) + 1
#define XMLRPC_NUMBER_OF_MEMBERS(Class) 0 xmlrpc_##Class##_FOREACH_MEMBER(XMLRPC_PLUS_ONE)

// Allow an optional XML RPC name to be passed as third parameter to 'X' (in the FOREACH_MEMBER declarations).
#define XMLRPC_DECLARE_NAME_3_ARGS(type, el, name) name,
#define XMLRPC_DECLARE_NAME_2_ARGS(type, el) XMLRPC_DECLARE_NAME_3_ARGS(type, el, #el)
#define XMLRPC_GET_4TH_ARG(arg1, arg2, arg3, arg4, ...) arg4
#define XMLRPC_DECLARE_NAME_MACRO_CHOOSER(...) XMLRPC_GET_4TH_ARG(__VA_ARGS__, XMLRPC_DECLARE_NAME_3_ARGS, XMLRPC_DECLARE_NAME_2_ARGS, )
#define XMLRPC_DECLARE_NAME(...) XMLRPC_DECLARE_NAME_MACRO_CHOOSER(__VA_ARGS__)(__VA_ARGS__)

// Used in template.{cxx,h}
#define XMLRPC_CLASSNAME_CREATE(cn) BOOST_PP_CAT(cn, Create)
#define XMLRPC_FOREACH_MEMBER(cn, m) BOOST_PP_CAT(xmlrpc_, BOOST_PP_CAT(cn, _FOREACH_MEMBER))(m)

namespace xmlrpc {
class ElementDecoder;
} // namespace xmlrpc
