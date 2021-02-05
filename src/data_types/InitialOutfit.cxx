#include "sys.h"
#include "InitialOutfit.h"
#include "protocols/xmlrpc/IgnoreElement.h"

constexpr size_t InitialOutfit::s_number_of_members;

std::array<char const*, InitialOutfit::s_number_of_members> InitialOutfit::s_member2name = {
  xmlrpc_InitialOutfit_FOREACH_MEMBER(XMLRPC_DECLARE_MEMBER_NAME)
};

xmlrpc::ElementDecoder* InitialOutfit::get_member_decoder(members member)
{
  switch (member)
  {
    xmlrpc_InitialOutfit_FOREACH_MEMBER(XMLRPC_CASE_RETURN_MEMBER_DECODER)
  }
}
