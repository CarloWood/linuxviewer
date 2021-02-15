#pragma once

#include "ModerationLevel.h"

class AgentAccess
{
 private:
  ModerationLevel m_agent_access;

 public:
  AgentAccess() : m_agent_access(moderation_level_unknown) { }

  void set_moderation_level(ModerationLevel agent_access) { m_agent_access = agent_access; }
  ModerationLevel get_moderation_level() const { return m_agent_access; }
};
