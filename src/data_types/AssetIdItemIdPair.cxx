#include "sys.h"
#include "AssetIdItemIdPair.h"
#include "evio/protocol/xmlrpc/create_member_decoder.h"
#ifdef CWDEBUG
#include <iostream>
#endif

evio::protocol::xmlrpc::ElementDecoder* AssetIdItemIdPair::create_member_decoder(members member)
{
  switch (member)
  {
    xmlrpc_AssetIdItemIdPair_FOREACH_MEMBER(XMLRPC_CASE_RETURN_MEMBER_DECODER)
  }
  AI_NEVER_REACHED
}

#ifdef CWDEBUG
void AssetIdItemIdPair::print_on(std::ostream& os) const
{
  os << '{';
  char const* prefix = "";
  xmlrpc_AssetIdItemIdPair_FOREACH_MEMBER(XMLRPC_WRITE_TO_OS);
  os << '}';
}
#endif
