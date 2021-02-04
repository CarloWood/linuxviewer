#include "sys.h"
#include "LoginFlags.h"

constexpr size_t LoginFlags::s_number_of_members;

#define xmlrpc_NAMES_ARE_THE_SAME(flags, type, el) #el,

std::array<char const*, LoginFlags::s_number_of_members> LoginFlags::s_member2name = {
  xmlrpc_LoginFlags_FOREACH_ELEMENT(xmlrpc_NAMES_ARE_THE_SAME)
};
