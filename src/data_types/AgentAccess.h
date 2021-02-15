#pragma once

#include "ModerationLevel.h"
#include <string_view>

class AgentAccess
{
 private:
  ModerationLevel m_agent_access;

 public:
  AgentAccess() : m_agent_access(moderation_level_unknown) { }

  void set_moderation_level(ModerationLevel agent_access) { m_agent_access = agent_access; }
  ModerationLevel get_moderation_level() const { return m_agent_access; }

  void assign_from_xmlrpc_string(std::string_view const& data);

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};
