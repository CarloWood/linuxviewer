#include "sys.h"
#include "SunCloudMoonTextures.h"
#include "protocols/xmlrpc/create_member_decoder.h"

xmlrpc::ElementDecoder* SunCloudMoonTextures::create_member_decoder(members member)
{
  switch (member)
  {
    xmlrpc_SunCloudMoonTextures_FOREACH_MEMBER(XMLRPC_CASE_RETURN_MEMBER_DECODER)
  }
}
