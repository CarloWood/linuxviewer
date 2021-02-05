#include "sys.h"
#include "Buddy.h"
#include "protocols/xmlrpc/create_member_decoder.h"

xmlrpc::ElementDecoder* Buddy::get_member_decoder(members member)
{
  switch (member)
  {
    xmlrpc_Buddy_FOREACH_MEMBER(XMLRPC_CASE_RETURN_MEMBER_DECODER)
  }
}
