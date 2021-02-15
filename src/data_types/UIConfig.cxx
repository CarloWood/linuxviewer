#include "sys.h"
#include "UIConfig.h"
#include "evio/protocol/xmlrpc/create_member_decoder.h"

xmlrpc::ElementDecoder* UIConfig::create_member_decoder(members member)
{
  switch (member)
  {
    xmlrpc_UIConfig_FOREACH_MEMBER(XMLRPC_CASE_RETURN_MEMBER_DECODER)
  }
}
