#include "sys.h"
#include "LoginResponse.h"
#include "xmlrpc/create_member_decoder.h"

namespace xmlrpc {

#if 0
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
#endif

ElementDecoder* LoginResponse::get_member_decoder(members member)
{
  switch (member)
  {
    xmlrpc_LoginResponse_FOREACH_MEMBER(XMLRPC_CASE_RETURN_MEMBER_DECODER)
  }
}

} // namespace xmlrpc
