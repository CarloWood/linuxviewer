#include "sys.h"
#include "AssetIdItemIdPair.h"
#include "evio/protocol/xmlrpc/create_member_decoder.h"

xmlrpc::ElementDecoder* AssetIdItemIdPair::create_member_decoder(members member)
{
  switch (member)
  {
    xmlrpc_AssetIdItemIdPair_FOREACH_MEMBER(XMLRPC_CASE_RETURN_MEMBER_DECODER)
  }
}
