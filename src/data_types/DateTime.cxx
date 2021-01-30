#include "sys.h"
#include "DateTime.h"
#include "utils/AIAlert.h"
#include "debug.h"
#include <cctype>       // std::isdigit
#include <sstream>
#include <iomanip>
#include <time.h>       // gmtime_r, timegm

namespace {

// The output for posix time 67767976233532799.
// Larger values even put a minus sign in front of this, but sizeof
// includes the terminating zero, so that has been taken into account ;).
constexpr size_t max_iso8601_string_len = sizeof("2147483647-12-31T23:59:59Z");

// Convert a series of digits into an int.
int to_int(std::string_view::const_iterator& start, std::string_view::const_iterator end)
{
  int result = 0;
  while (start != end && std::isdigit(*start))
  {
    result *= 10;
    result += *start++ - '0';
  }
  return result;
}

int next_two_digits(std::string_view::const_iterator& start, std::string_view::const_iterator end, char c)
{
  if (start == end || !isdigit(*start))
    return -1;
  int result = 10 * (*start++ - '0');
  if (start == end || !isdigit(*start))
    return -1;
  result += *start++ - '0';
  if (start == end || *start++ != c)
    return -1;
  return result;
}

} // namespace

void DateTime::assign_from_iso8601_string(std::string_view const& data)
{
  struct tm date_time{};
  auto current_pos = data.begin();
  date_time.tm_year = to_int(current_pos, data.end()) - 1900;
  bool parse_error = current_pos == data.begin() || *current_pos != '-';
  if (!parse_error)
  {
    ++current_pos;      // Skip '-'.
    parse_error =
        (date_time.tm_mon  = next_two_digits(current_pos, data.end(), '-')) == -1 ||
        (date_time.tm_mday = next_two_digits(current_pos, data.end(), 'T')) == -1 ||
        (date_time.tm_hour = next_two_digits(current_pos, data.end(), ':')) == -1 ||
        (date_time.tm_min  = next_two_digits(current_pos, data.end(), ':')) == -1 ||
        (date_time.tm_sec  = next_two_digits(current_pos, data.end(), 'Z')) == -1 ||
        current_pos != data.end();
    --date_time.tm_mon; // Runs from 0..11
  }
  if (parse_error)
    THROW_FALERTE("Failed to parse ISO8601 date/time \"[DATA]\"", AIArgs("[DATA]", data));
  date_time.tm_isdst = 0;       // Zulu time has no DST.
  m_posix_time = timegm(&date_time);
  if (m_posix_time == -1)
    THROW_ALERTE("mktime: could not convert \"[DATETIME]\" to a time_t!", AIArgs("[DATETIME]", std::put_time(&date_time, "%FT%TZ")));
}

std::string DateTime::to_iso8601_string() const
{
  std::ostringstream buf2;
  print_on(buf2);
  return buf2.str();
}

void DateTime::print_on(std::ostream& os) const
{
  struct tm date_time_buf;
  os << std::put_time(gmtime_r(&m_posix_time, &date_time_buf), "%FT%TZ");
}
