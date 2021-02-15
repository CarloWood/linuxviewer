#include "sys.h"
#include "initialize.h"
#include "data_types/AgentAccess.h"
#include "data_types/Gender.h"
#include "utils/macros.h"
#include "utils/AIAlert.h"
#include "utils/c_escape.h"
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>
#include <cctype>       // std::isspace
#include "debug.h"

namespace xmlrpc {

void initialize(Vector3d& vec, std::string_view const& vector3d_data)
{
  // The format of the string is "[r0.8928223,r0.450409,r0]".
  // Hence, Vector3d does not need xml unescaping, since it does not contain any of '"<>&.
  boost::iostreams::stream<boost::iostreams::basic_array_source<char>> stream(vector3d_data.begin(), vector3d_data.end());
  bool parse_error = false;
  char expected = '[';
  char c;
  int i = 0;
  for (;;)
  {
    // Read next expected character.
    stream >> c;
    if (AI_UNLIKELY(stream.eof()) || AI_UNLIKELY(c != expected))
    {
      // Allow white space in front of expected characters.
      // This therefore allows " [ r0.8928223 , r0.450409, r0 ]".
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
      expected = (i == 2) ? ']' : ',';
      stream >> vec[i];
    }
    else
      break;
  }
  if (AI_UNLIKELY(parse_error))
    THROW_FALERT("Parse error while decoding \"[DATA]\"", AIArgs("[DATA]", vector3d_data));
}

void initialize(RegionHandle& region_handle, std::string_view const& data)
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
  region_handle.set_position(x, y);
}

void initialize(AgentAccess& agent_access, std::string_view const& data)
{
  bool parse_error = data.size() != 1;
  if (!parse_error)
  {
    if (data[0] == 'P')
      agent_access.set_moderation_level(moderation_level_PG);
    else if (data[0] == 'M')
      agent_access.set_moderation_level(moderation_level_moderate);
    else if (data[0] == 'A')
      agent_access.set_moderation_level(moderation_level_adult);
    else
      parse_error = true;
  }
  if (parse_error)
    THROW_ALERT("Invalid AgentAccess '[LEVEL]'", AIArgs("[LEVEL]", utils::print_using(data, utils::c_escape)));
}

void initialize(Gender& gender, std::string_view const& data)
{
  if (data == "female")
    gender.set_gender(gender_female);
  else if (data == "male")
    gender.set_gender(gender_male);
  else
    THROW_ALERT("Invalid Gender '[GENDER]'", AIArgs("[GENDER]", utils::print_using(data, utils::c_escape)));
}

} // namespace xmlrpc
