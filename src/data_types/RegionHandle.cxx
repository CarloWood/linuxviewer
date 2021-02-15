#include "sys.h"
#include "RegionHandle.h"
#include "utils/macros.h"
#include "utils/AIAlert.h"
#include <boost/iostreams/stream.hpp>
#ifdef CWDEBUG
#include <iostream>
#endif

void RegionHandle::assign_from_xmlrpc_string(std::string_view const& data)
{
  // The format of the string is "[r254208,r261120]" even though these numbers are not reals, but integers.
  // Hence, RegionHandle does not need xml unescaping, since it does not contain any of '"<>&.
  boost::iostreams::stream<boost::iostreams::basic_array_source<char>> stream(data.begin(), data.end());
  bool parse_error = false;
  char expected = '[';
  char c;
  int i = 0;
  int x, y;
  for (;;)
  {
    // Read next expected character.
    stream >> c;
    if (AI_UNLIKELY(stream.eof()) || AI_UNLIKELY(c != expected))
    {
      // Allow white space in front of expected characters.
      // This therefore allows " [ r254208 , r261120 ]".
      if (std::isspace(c))
        continue;
      parse_error = true;
      break;
    }
    if (expected == '[')
      expected = 'r';
    else if (expected == ',')
    {
      ++i;
      expected = 'r';
    }
    else if (expected == 'r')
    {
      expected = (i == 1) ? ']' : ',';
      if (i == 0)
        stream >> x;
      else
        stream >> y;
    }
    else
      break;
  }
  if (AI_UNLIKELY(parse_error))
    THROW_FALERT("Parse error while decoding \"[DATA]\"", AIArgs("[DATA]", data));
  set_position(x, y);
}

#ifdef CWDEBUG
void RegionHandle::print_on(std::ostream& os) const
{
  os << "{x:" << m_x << ", y:" << m_y << '}';
}
#endif
