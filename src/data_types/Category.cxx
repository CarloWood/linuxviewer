#include "sys.h"
#include "Category.h"
#include "protocols/xmlrpc/create_member_decoder.h"

xmlrpc::ElementDecoder* Category::get_member_decoder(members member)
{
  switch (member)
  {
    xmlrpc_Category_FOREACH_MEMBER(XMLRPC_CASE_RETURN_MEMBER_DECODER)
  }
}
