#include "sys.h"
#include "InitialOutfit.h"
#include "evio/protocol/xmlrpc/create_member_decoder.h"

xmlrpc::ElementDecoder* InitialOutfit::create_member_decoder(members member)
{
  switch (member)
  {
    xmlrpc_InitialOutfit_FOREACH_MEMBER(XMLRPC_CASE_RETURN_MEMBER_DECODER)
  }
}
