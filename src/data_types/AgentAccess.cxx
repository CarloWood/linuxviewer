#include "sys.h"
#include "AgentAccess.h"
#include "utils/AIAlert.h"
#include "utils/print_using.h"
#include "utils/c_escape.h"
#ifdef CWDEBUG
#include <iostream>
#endif

void AgentAccess::assign_from_xmlrpc_string(std::string_view const& data)
{
  bool parse_error = data.size() != 1;
  if (!parse_error)
  {
    if (data[0] == 'P')
      set_moderation_level(moderation_level_PG);
    else if (data[0] == 'M')
      set_moderation_level(moderation_level_moderate);
    else if (data[0] == 'A')
      set_moderation_level(moderation_level_adult);
    else
      parse_error = true;
  }
  if (parse_error)
    THROW_ALERT("Invalid AgentAccess '[LEVEL]'", AIArgs("[LEVEL]", utils::print_using(data, utils::c_escape)));
}

#ifdef CWDEBUG
void AgentAccess::print_on(std::ostream& os) const
{
  os << '{' << m_agent_access << '}';
}
#endif
