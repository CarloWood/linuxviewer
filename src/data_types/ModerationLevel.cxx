#include "sys.h"
#include "ModerationLevel.h"
#ifdef CWDEBUG
#include "utils/macros.h"
#include <iostream>
#endif

#ifdef CWDEBUG
std::ostream& operator<<(std::ostream& os, ModerationLevel moderation_level)
{
  switch (moderation_level)
  {
    case moderation_level_unknown:
      os << "moderation_level_unknown";
      break;
    case moderation_level_PG:
      os << "moderation_level_PG";
      break;
    case moderation_level_moderate:
      os << "moderation_level_moderate";
      break;
    case moderation_level_adult:
      os << "moderation_level_adult";
      break;
  }
  return os;
}
#endif
