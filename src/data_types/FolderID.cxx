#include "sys.h"
#include "FolderID.h"
#include "protocols/xmlrpc/create_member_decoder.h"

xmlrpc::ElementDecoder* FolderID::create_member_decoder(members member)
{
  switch (member)
  {
    xmlrpc_FolderID_FOREACH_MEMBER(XMLRPC_CASE_RETURN_MEMBER_DECODER)
  }
}
