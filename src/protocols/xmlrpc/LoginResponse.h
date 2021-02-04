#pragma once

#include "protocols/XML_RPC_SingleStructResponse.h"
#include "data_types/UUID.h"
#include "data_types/URI.h"
#include "data_types/Vector3d.h"
#include "data_types/AgentAccess.h"
#include "data_types/LoginFlags.h"
#include "data_types/InitialOutfit.h"
#include "data_types/AssetIdItemIdPair.h"
#include "utils/Dictionary.h"
#include <vector>

#define DECLARE_UNKNOWN(T) \
  struct T { \
    enum members { }; \
    static constexpr size_t s_number_of_members = 0; \
  }; \
  template<> class XML_RPC_MemberWrapper<T> : public XML_RPC_UnknownMemberWrapper \
  { \
    using XML_RPC_UnknownMemberWrapper::XML_RPC_UnknownMemberWrapper; \
  }

DECLARE_UNKNOWN(Buddy);
DECLARE_UNKNOWN(Category);
DECLARE_UNKNOWN(SunCloudMoonTextures);
DECLARE_UNKNOWN(RegionPositionLookAt);
DECLARE_UNKNOWN(AgentID);
DECLARE_UNKNOWN(FolderID);
DECLARE_UNKNOWN(InventoryFolder);
DECLARE_UNKNOWN(UIConfig);

namespace xmlrpc {

#define xmlrpc_LoginResponse_FOREACH_ELEMENT(X) \
  X(0, AgentAccess,                     agent_access) \
  X(0, AgentAccess,                     agent_access_max) \
  X(0, UUID,                            agent_id) \
  X(0, std::vector<Buddy>,              buddy_list) \
  X(0, int32_t,                         circuit_code) \
  X(0, std::vector<Category>,           classified_categories) \
  X(0, std::string,                     currency) \
  X(0, URI,                             destination_guide_url) \
  X(0, std::vector<Category>,           event_categories) \
  X(0, std::vector<Unknown>,            event_notifications) \
  X(0, std::string,                     first_name) \
  X(0, std::vector<AssetIdItemIdPair>,  gestures) \
  X(1, SunCloudMoonTextures,            global_textures) \
  X(0, RegionPositionLookAt,            home) \
  X(0, int32_t,                         http_port) \
  X(1, InitialOutfit,                   initial_outfit) \
  X(1, AgentID,                         inventory_lib_owner) \
  X(1, FolderID,                        inventory_lib_root) \
  X(1, FolderID,                        inventory_root) \
  X(0, std::vector<InventoryFolder>,    inventory_skeleton) \
  X(0, std::vector<InventoryFolder>,    inventory_skel_lib) \
  X(0, std::string,                     last_name) \
  X(0, bool,                            login) \
  X(1, LoginFlags,                      login_flags) \
  X(0, Vector3d,                        look_at) \
  X(0, URI,                             map_server_url) \
  X(0, int32_t,                         max_agent_groups) \
  X(0, std::string,                     message) \
  X(0, UUID,                            real_id) \
  X(0, int32_t,                         region_size_x) \
  X(0, int32_t,                         region_size_y) \
  X(0, int32_t,                         region_x) \
  X(0, int32_t,                         region_y) \
  X(0, int32_t,                         seconds_since_epoch) \
  X(0, UUID,                            secure_session_id) \
  X(0, URI,                             seed_capability) \
  X(0, UUID,                            session_id) \
  X(0, std::string,                     sim_ip) \
  X(0, int32_t,                         sim_port) \
  X(0, std::string,                     start_location) \
  X(1, UIConfig,                        ui_config)

class LoginResponse : public XML_RPC_SingleStructResponse
{
 public:
  LoginResponse();

  #define xmlrpc_LoginResponse_ENUMERATOR_NAME_COMMA(flags, type, el) member_##el,

  enum members {
    xmlrpc_LoginResponse_FOREACH_ELEMENT(xmlrpc_LoginResponse_ENUMERATOR_NAME_COMMA)
  };

  #define xmlrpc_LoginResponse_DECLARATION(flags, type, el) type m_##el;

  xmlrpc_LoginResponse_FOREACH_ELEMENT(xmlrpc_LoginResponse_DECLARATION)

  static constexpr size_t s_number_of_members = member_ui_config + 1;
  static std::array<char const*, s_number_of_members> s_member2name;

 private:
  utils::Dictionary<members, int> m_dictionary;
#ifdef CWDEBUG
  char const* lm_last_name;      // Name of the members enumerator corresponding to the last name of the call to get_member.
#endif

  XML_RPC_Response* get_member(std::string_view const& name) override;

#ifdef CWDEBUG
  char const* member2str(LoginResponse::members member);

  void got_member_type(data_type type, char const* struct_name) override;
#endif
};

} // namespace xmlrpc
