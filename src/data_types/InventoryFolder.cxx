#include "sys.h"
#include "InventoryFolder.h"
#include "evio/protocol/xmlrpc/create_member_decoder.h"
#ifdef CWDEBUG
#include <iostream>
#endif

evio::protocol::xmlrpc::ElementDecoder* InventoryFolder::create_member_decoder(members member)
{
  switch (member)
  {
    xmlrpc_InventoryFolder_FOREACH_MEMBER(XMLRPC_CASE_RETURN_MEMBER_DECODER)
  }
  AI_NEVER_REACHED
}

#ifdef CWDEBUG
void InventoryFolder::print_on(std::ostream& os) const
{
  os << '{';
  char const* prefix = "";
  xmlrpc_InventoryFolder_FOREACH_MEMBER(XMLRPC_WRITE_TO_OS);
  os << '}';
}
#endif
