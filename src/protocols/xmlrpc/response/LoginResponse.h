#pragma once

#include "protocols/xmlrpc/initialize.h"
#include "data_types/AgentAccess.h"
#include "data_types/UUID.h"
#include "data_types/Buddy.h"
#include "data_types/Category.h"
#include "data_types/URI.h"
#include "data_types/AssetIdItemIdPair.h"
#include "data_types/SunCloudMoonTextures.h"
#include "data_types/RegionPositionLookAt.h"
#include "data_types/InitialOutfit.h"
#include "data_types/AgentID.h"
#include "data_types/FolderID.h"
#include "data_types/InventoryFolder.h"
#include "data_types/LoginFlags.h"
#include "data_types/Vector3d.h"
#include "data_types/UIConfig.h"
#include "evio/protocol/xmlrpc/SingleStructResponse.h"
#include "evio/protocol/xmlrpc/macros.h"

// FIXME: event_notifications has unknown struct (not std::string)
#define xmlrpc_LoginResponse_FOREACH_MEMBER(X) \
  X(AgentAccess,                     agent_access) \
  X(AgentAccess,                     agent_access_max) \
  X(UUID,                            agent_id) \
  X(std::vector<Buddy>,              buddy_list) \
  X(int32_t,                         circuit_code) \
  X(std::vector<Category>,           classified_categories) \
  X(std::string,                     currency) \
  X(URI,                             destination_guide_url) \
  X(std::vector<Category>,           event_categories) \
  X(std::vector<std::string>,        event_notifications) \
  X(std::string,                     first_name) \
  X(std::vector<AssetIdItemIdPair>,  gestures) \
  X(SunCloudMoonTextures,            global_textures) \
  X(RegionPositionLookAt,            home) \
  X(int32_t,                         http_port) \
  X(InitialOutfit,                   initial_outfit) \
  X(AgentID,                         inventory_lib_owner) \
  X(FolderID,                        inventory_lib_root) \
  X(FolderID,                        inventory_root) \
  X(std::vector<InventoryFolder>,    inventory_skeleton) \
  X(std::vector<InventoryFolder>,    inventory_skel_lib) \
  X(std::string,                     last_name) \
  X(bool,                            login) \
  X(LoginFlags,                      login_flags) \
  X(Vector3d,                        look_at) \
  X(URI,                             map_server_url) \
  X(int32_t,                         max_agent_groups) \
  X(std::string,                     message) \
  X(UUID,                            real_id) \
  X(int32_t,                         region_size_x) \
  X(int32_t,                         region_size_y) \
  X(int32_t,                         region_x) \
  X(int32_t,                         region_y) \
  X(int32_t,                         seconds_since_epoch) \
  X(UUID,                            secure_session_id) \
  X(URI,                             seed_capability) \
  X(UUID,                            session_id) \
  X(std::string,                     sim_ip) \
  X(int32_t,                         sim_port) \
  X(std::string,                     start_location) \
  X(UIConfig,                        ui_config)

namespace xmlrpc {

class LoginResponseData
{
 private:
  xmlrpc_LoginResponse_FOREACH_MEMBER(XMLRPC_DECLARE_MEMBER)

 public:
  enum members {
    xmlrpc_LoginResponse_FOREACH_MEMBER(XMLRPC_DECLARE_ENUMERATOR)
  };

  evio::protocol::xmlrpc::ElementDecoder* create_member_decoder(members member);

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

struct LoginResponse : public LoginResponseData, public evio::protocol::xmlrpc::SingleStructResponse<LoginResponseData>
{
  LoginResponse() : evio::protocol::xmlrpc::SingleStructResponse<LoginResponseData>(static_cast<LoginResponseData&>(*this)) { }
};

} // namespace xmlrpc
