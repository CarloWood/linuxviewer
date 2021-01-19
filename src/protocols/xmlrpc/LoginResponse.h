#pragma once

#include <array>

namespace xmlrpc {

class LoginResponse
{
 public:
  enum members {
    agent_access,
    agent_access_max,
    agent_id,
    buddy_list,
    circuit_code,
    classified_categories,
    currency,
    destination_guide_url,
    event_categories,
    event_notifications,
    first_name,
    gestures,
    global_textures,
    home,
    http_port,
    initial_outfit,
    inventory_lib_owner,
    inventory_lib_root,
    inventory_root,
    inventory_skeleton,
    inventory_skel_lib,
    last_name,
    login,
    login_flags,
    look_at,
    map_server_url,
    max_agent_groups,
    message,
    real_id,
    region_size_x,
    region_size_y,
    region_x,
    region_y,
    seconds_since_epoch,
    secure_session_id,
    seed_capability,
    session_id,
    sim_ip,
    sim_port,
    start_location,
    ui_config,
  };

  static constexpr size_t s_number_of_members = ui_config + 1;
  static std::array<char const*, s_number_of_members> const s_member2name;
};

} // namespace xmlrpc
