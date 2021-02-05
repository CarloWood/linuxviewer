#include "sys.h"
#include "UIConfig.h"
#include "protocols/xmlrpc/create_member_decoder.h"

xmlrpc::ElementDecoder* UIConfig::get_member_decoder(members member)
{
  switch (member)
  {
    xmlrpc_UIConfig_FOREACH_MEMBER(XMLRPC_CASE_RETURN_MEMBER_DECODER)
  }
}
