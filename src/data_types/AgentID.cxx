#include "sys.h"
#include "AgentID.h"
#include "evio/protocol/xmlrpc/create_member_decoder.h"
#ifdef CWDEBUG
#include <iostream>
#endif

evio::protocol::xmlrpc::ElementDecoder* AgentID::create_member_decoder(members member)
{
  switch (member)
  {
    xmlrpc_AgentID_FOREACH_MEMBER(XMLRPC_CASE_RETURN_MEMBER_DECODER)
  }
}

#ifdef CWDEBUG
void AgentID::print_on(std::ostream& os) const
{
  os << '{';
  char const* prefix = "";
  xmlrpc_AgentID_FOREACH_MEMBER(XMLRPC_WRITE_TO_OS);
  os << '}';
}
#endif
