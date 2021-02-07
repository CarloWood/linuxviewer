#include "sys.h"
#include "AgentID.h"
#include "protocols/xmlrpc/create_member_decoder.h"

xmlrpc::ElementDecoder* AgentID::create_member_decoder(members member)
{
  switch (member)
  {
    xmlrpc_AgentID_FOREACH_MEMBER(XMLRPC_CASE_RETURN_MEMBER_DECODER)
  }
}
