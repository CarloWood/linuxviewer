#pragma once

#include <ctime>
#include <string_view>
#include <iosfwd>

class DateTime
{
 private:
  std::time_t m_posix_time;     // POSIX time: the number of days since the epoch (00:00:00 UTC on 1 January 1970)
                                // times 86400, plus the number of seconds since the last midnight (UTC).
                                //
                                // In other words, this value is NOT a continuous clock, but rather a
                                // calendar clock that makes conversion to a date easy.
                                //
                                // Alternatively, you can view it as the number of SI seconds since the
                                // epoch MINUS the number of leap seconds since that moment.

 public:
  // Construct an uninitialized DateTime object.
  DateTime() { }
  DateTime(std::time_t posix_time) : m_posix_time(posix_time) { }

  void assign_from_iso8601_string(std::string_view const& data);

  std::string to_iso8601_string() const;

  void print_on(std::ostream& os) const;
};
