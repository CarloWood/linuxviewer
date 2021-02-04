#include "sys.h"
#include "LoginResponse.h"
#include "xmlrpc/IgnoreElement.h"
#include "xmlrpc/StructDecoder.h"
#include "debug.h"

namespace xmlrpc {

constexpr size_t LoginResponse::s_number_of_members;

std::array<char const*, LoginResponse::s_number_of_members> LoginResponse::s_member2name = {
  "agent_access",
  "agent_access_max",
  "agent_id",
  "buddy-list",
  "circuit_code",
  "classified_categories",
  "currency",
  "destination_guide_url",
  "event_categories",
  "event_notifications",
  "first_name",
  "gestures",
  "global-textures",
  "home",
  "http_port",
  "initial-outfit",
  "inventory-lib-owner",
  "inventory-lib-root",
  "inventory-root",
  "inventory-skeleton",
  "inventory-skel-lib",
  "last_name",
  "login",
  "login-flags",
  "look_at",
  "map-server-url",
  "max-agent-groups",
  "message",
  "real_id",
  "region_size_x",
  "region_size_y",
  "region_x",
  "region_y",
  "seconds_since_epoch",
  "secure_session_id",
  "seed_capability",
  "session_id",
  "sim_ip",
  "sim_port",
  "start_location",
  "ui-config",
};

LoginResponse::LoginResponse()
{
  for (size_t i = 0; i < s_number_of_members; ++i)
    m_dictionary.add(static_cast<members>(i), s_member2name[i]);
}

#ifdef CWDEBUG
char const* LoginResponse::member2str(LoginResponse::members member)
{
  switch (member)
  {
    xmlrpc_LoginResponse_FOREACH_MEMBER(XMLRPC_CASE_RETURN)
  }
  // Never reached.
  return "";
}
#endif

ElementDecoder* LoginResponse::get_member_decoder(std::string_view const& name)
{
  int index = m_dictionary.index(name);
  if (index >= s_number_of_members)
    return &IgnoreElement::s_ignore_element;
  members member = static_cast<members>(index);
  Debug(lm_last_name = member2str(member));
  switch (member)
  {
    xmlrpc_LoginResponse_FOREACH_MEMBER(XMLRPC_CASE_RETURN_MEMBER_DECODER)
  }
}

#ifdef CWDEBUG
void LoginResponse::got_member_type(data_type type, char const* struct_name)
{
  char const* type_name = (type == data_type_struct) ? struct_name : data_type_to_str[type];
  Dout(dc::notice, "=== " << type_name << ' '<< lm_last_name << ';');
}
#endif

} // namespace xmlrpc
