#include "sys.h"
#include "InventoryFolder.h"
#include "protocols/xmlrpc/create_member_decoder.h"

xmlrpc::ElementDecoder* InventoryFolder::get_member_decoder(members member)
{
  switch (member)
  {
    xmlrpc_InventoryFolder_FOREACH_MEMBER(XMLRPC_CASE_RETURN_MEMBER_DECODER)
  }
}
