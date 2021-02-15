#pragma once

#ifdef CWDEBUG
#include <iosfwd>
#endif

enum ModerationLevel
{
  moderation_level_unknown,
  moderation_level_PG,          // 'P'
  moderation_level_moderate,    // 'M'
  moderation_level_adult        // 'A'
};

#ifdef CWDEBUG
std::ostream& operator<<(std::ostream& os,  ModerationLevel moderation_level);
#endif
