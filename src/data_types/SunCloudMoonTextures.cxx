#include "sys.h"
#include "SunCloudMoonTextures.h"
#include "evio/protocol/xmlrpc/create_member_decoder.h"
#ifdef CWDEBUG
#include <iostream>
#endif

evio::protocol::xmlrpc::ElementDecoder* SunCloudMoonTextures::create_member_decoder(members member)
{
  switch (member)
  {
    xmlrpc_SunCloudMoonTextures_FOREACH_MEMBER(XMLRPC_CASE_RETURN_MEMBER_DECODER)
  }
}

#ifdef CWDEBUG
void SunCloudMoonTextures::print_on(std::ostream& os) const
{
  os << '{';
  char const* prefix = "";
  xmlrpc_SunCloudMoonTextures_FOREACH_MEMBER(XMLRPC_WRITE_TO_OS);
  os << '}';
}
#endif
