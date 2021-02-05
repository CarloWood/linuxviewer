#include "sys.h"
#include "LoginFlags.h"
#include "protocols/xmlrpc/IgnoreElement.h"

constexpr size_t LoginFlags::s_number_of_members;

std::array<char const*, LoginFlags::s_number_of_members> LoginFlags::s_member2name = {
  xmlrpc_LoginFlags_FOREACH_MEMBER(XMLRPC_DECLARE_MEMBER_NAME)
};

xmlrpc::ElementDecoder* LoginFlags::get_member_decoder(members member)
{
  switch (member)
  {
    xmlrpc_LoginFlags_FOREACH_MEMBER(XMLRPC_CASE_RETURN_MEMBER_DECODER)
  }
}
