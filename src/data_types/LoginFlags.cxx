#include "sys.h"
#include "LoginFlags.h"
#include "protocols/xmlrpc/create_member_decoder.h"

xmlrpc::ElementDecoder* LoginFlags::get_member_decoder(members member)
{
  switch (member)
  {
    xmlrpc_LoginFlags_FOREACH_MEMBER(XMLRPC_CASE_RETURN_MEMBER_DECODER)
  }
}
