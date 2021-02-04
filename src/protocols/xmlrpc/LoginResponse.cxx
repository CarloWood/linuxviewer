#include "sys.h"
#include "LoginResponse.h"
#include "debug.h"

template<>
class XML_RPC_StructWrapper<LoginFlags> : public XML_RPC_StructWrapperBase<LoginFlags>
{
 protected:
  XML_RPC_Response* get_member(std::string_view const& name) override;

#ifdef CWDEBUG
  XML_RPC_Response* get_struct() override
  {
    m_struct_name = "LoginFlags";
    return this;
  }
#endif

  using XML_RPC_StructWrapperBase<LoginFlags>::XML_RPC_StructWrapperBase;
};

XML_RPC_Response* XML_RPC_StructWrapper<LoginFlags>::get_member(std::string_view const& name)
{
  int index = m_dictionary.index(name);
  if (index >= LoginFlags::s_number_of_members)
    return &XML_RPC_IgnoreElement::s_ignore_element;
  LoginFlags::members member = static_cast<LoginFlags::members>(index);
  switch (member)
  {
    #define xmlrpc_LoginFlags_CREATE_XMLRPC_MEMBER_WRAPPER(flags, type, el) \
    case LoginFlags::member_##el: \
      return create_xmlrpc_member_wrapper(m_member.m_##el, flags COMMA_CWDEBUG_ONLY(#el));

    xmlrpc_LoginFlags_FOREACH_ELEMENT(xmlrpc_LoginFlags_CREATE_XMLRPC_MEMBER_WRAPPER)
  }
}

template<>
XML_RPC_Response* create_xmlrpc_member_wrapper(LoginFlags& member, int flags COMMA_CWDEBUG_ONLY(char const* name))
{
  //FIXME: this is not freed anywhere yet.
  return new XML_RPC_StructWrapper<LoginFlags>{member, flags COMMA_CWDEBUG_ONLY(name)};
}

template<>
class XML_RPC_StructWrapper<InitialOutfit> : public XML_RPC_StructWrapperBase<InitialOutfit>
{
 protected:
  XML_RPC_Response* get_member(std::string_view const& name) override;

#ifdef CWDEBUG
  XML_RPC_Response* get_struct() override
  {
    m_struct_name = "InitialOutfit";
    return XML_RPC_StructWrapperBase<InitialOutfit>::get_struct();
  }
#endif

  using XML_RPC_StructWrapperBase<InitialOutfit>::XML_RPC_StructWrapperBase;
};

XML_RPC_Response* XML_RPC_StructWrapper<InitialOutfit>::get_member(std::string_view const& name)
{
  int index = m_dictionary.index(name);
  if (index >= InitialOutfit::s_number_of_members)
    return &XML_RPC_IgnoreElement::s_ignore_element;
  InitialOutfit::members member = static_cast<InitialOutfit::members>(index);
  switch (member)
  {
    #define xmlrpc_InitialOutfit_CREATE_XMLRPC_MEMBER_WRAPPER(flags, type, el) \
    case InitialOutfit::member_##el: \
      return create_xmlrpc_member_wrapper(m_member.m_##el, flags COMMA_CWDEBUG_ONLY(#el));

    xmlrpc_InitialOutfit_FOREACH_ELEMENT(xmlrpc_InitialOutfit_CREATE_XMLRPC_MEMBER_WRAPPER)
  }
}

template<>
XML_RPC_Response* create_xmlrpc_member_wrapper(InitialOutfit& member, int flags COMMA_CWDEBUG_ONLY(char const* name))
{
  //FIXME: this is not freed anywhere yet.
  return new XML_RPC_StructWrapper<InitialOutfit>{member, flags COMMA_CWDEBUG_ONLY(name)};
}

template<>
class XML_RPC_ArrayWrapper<AssetIdItemIdPair> : public XML_RPC_ArrayWrapperBase<AssetIdItemIdPair>
{
 protected:
  XML_RPC_Response* get_member(std::string_view const& name) override;

#ifdef CWDEBUG
  XML_RPC_Response* get_struct() override
  {
    m_struct_name = "AssetIdItemIdPair";
    return this;
  }
#endif

  using XML_RPC_ArrayWrapperBase<AssetIdItemIdPair>::XML_RPC_ArrayWrapperBase;
};

XML_RPC_Response* XML_RPC_ArrayWrapper<AssetIdItemIdPair>::get_member(std::string_view const& name)
{
  int index = m_dictionary.index(name);
  if (index >= AssetIdItemIdPair::s_number_of_members)
    return &XML_RPC_IgnoreElement::s_ignore_element;
  AssetIdItemIdPair::members member = static_cast<AssetIdItemIdPair::members>(index);
  switch (member)
  {
    #define xmlrpc_AssetIdItemIdPair_CREATE_XMLRPC_MEMBER_WRAPPER(flags, type, el) \
    case AssetIdItemIdPair::member_##el: \
      return create_xmlrpc_member_wrapper(m_array_element->m_##el, flags COMMA_CWDEBUG_ONLY(#el));

    xmlrpc_AssetIdItemIdPair_FOREACH_ELEMENT(xmlrpc_AssetIdItemIdPair_CREATE_XMLRPC_MEMBER_WRAPPER)
  }
}

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
    #define xmlrpc_CASE_RETURN(flags, type, el) \
      case member_##el: \
        return #el;
    xmlrpc_LoginResponse_FOREACH_ELEMENT(xmlrpc_CASE_RETURN)
  }
  // Never reached.
  return "";
}
#endif

XML_RPC_Response* LoginResponse::get_member(std::string_view const& name)
{
  int index = m_dictionary.index(name);
  if (index >= s_number_of_members)
    return &XML_RPC_IgnoreElement::s_ignore_element;
  members member = static_cast<members>(index);
#ifdef CWDEBUG
  lm_last_name = member2str(member);
#endif
  switch (member)
  {
    #define xmlrpc_LoginResponse_CREATE_XMLRPC_MEMBER_WRAPPER(flags, type, el) \
      case member_##el: \
        return create_xmlrpc_member_wrapper(m_##el, flags COMMA_CWDEBUG_ONLY(#el));

    xmlrpc_LoginResponse_FOREACH_ELEMENT(xmlrpc_LoginResponse_CREATE_XMLRPC_MEMBER_WRAPPER)
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
