#include "sys.h"
#include "Category.h"
#include "evio/protocol/xmlrpc/create_member_decoder.h"

xmlrpc::ElementDecoder* Category::create_member_decoder(members member)
{
  switch (member)
  {
    xmlrpc_Category_FOREACH_MEMBER(XMLRPC_CASE_RETURN_MEMBER_DECODER)
  }
}
