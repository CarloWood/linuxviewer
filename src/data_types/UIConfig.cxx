#include "sys.h"
#include "UIConfig.h"
#include "evio/protocol/xmlrpc/create_member_decoder.h"
#ifdef CWDEBUG
#include <iostream>
#endif

evio::protocol::xmlrpc::ElementDecoder* UIConfig::create_member_decoder(members member)
{
  switch (member)
  {
    xmlrpc_UIConfig_FOREACH_MEMBER(XMLRPC_CASE_RETURN_MEMBER_DECODER)
  }
}

#ifdef CWDEBUG
void UIConfig::print_on(std::ostream& os) const
{
  os << '{';
  char const* prefix = "";
  xmlrpc_UIConfig_FOREACH_MEMBER(XMLRPC_WRITE_TO_OS);
  os << '}';
}
#endif
