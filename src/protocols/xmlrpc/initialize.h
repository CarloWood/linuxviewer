#pragma once

#include "utils/AIAlert.h"
#include "utils/print_using.h"
#include "utils/c_escape.h"
#include "data_types/BinaryData.h"
#include "data_types/DateTime.h"
#include "data_types/URI.h"
#include "data_types/UUID.h"
#include "data_types/Vector3d.h"
#include "data_types/ModerationLevel.h"
#include "data_types/AgentAccess.h"
#include "data_types/Gender.h"
#include "data_types/RegionPositionLookAt.h"
#include <boost/archive/iterators/xml_unescape.hpp>
#include <charconv>

namespace xmlrpc {

inline void initialize(BinaryData& binary_data, std::string_view const& base64_data)
{
  // Base64 does not need xml unescaping, since it does not contain any of '"<>&.
  binary_data.assign_from_base64(base64_data);
}

inline void initialize(DateTime& date_time, std::string_view const& iso8601_data)
{
  // ISO8601 does not need xml unescaping, since it does not contain any of '"<>&.
  date_time.assign_from_iso8601_string(iso8601_data);
}

inline void initialize(URI& uri, std::string_view const& uri_data)
{
  using xml_unescape_t = boost::archive::iterators::xml_unescape<std::string_view::const_iterator>;
  uri.assign(xml_unescape_t(uri_data.begin()), xml_unescape_t(uri_data.end()));
}

inline void initialize(UUID& uuid, std::string_view const& uuid_data)
{
  // UUID does not need xml unescaping, since it does not contain any of '"<>&.
  uuid.assign_from_string(uuid_data);
}

void initialize(Vector3d& vec, std::string_view const& vector3d_data);

inline void initialize(int32_t& value, std::string_view const& int_data)
{
  auto result = std::from_chars(int_data.begin(), int_data.end(), value);
  if (result.ec == std::errc::invalid_argument || result.ptr != int_data.end())
  {
    THROW_ALERTC(result.ec, "Invalid characters [[DATA]] for integer", AIArgs("[DATA]", utils::print_using(int_data, utils::c_escape)));
  }
}

inline void initialize(double& value, std::string_view const& double_data)
{
  std::string data{double_data};
  try
  {
    value = std::stod(data);
  }
  catch (std::invalid_argument const& error)
  {
    THROW_ALERT("Invalid characters [[DATA]] for floating point", AIArgs("[DATA]", utils::print_using(double_data, utils::c_escape)));
  }
  catch (std::out_of_range  const& error)
  {
    THROW_ALERT("Data [[DATA]] is out of range for a double", AIArgs("[DATA]", utils::print_using(double_data, utils::c_escape)));
  }
}

inline void initialize(bool& value, std::string_view const& bool_data)
{
  if (bool_data == "true" || bool_data == "Y")
    value = true;
  else if (bool_data == "false" || bool_data == "N")
    value = false;
  else
  {
    THROW_ALERT("Invalid characters [[DATA]] for boolean", AIArgs("[DATA]", utils::print_using(bool_data, utils::c_escape)));
  }
}

inline void initialize(std::string& value, std::string_view const& string_data)
{
  value = string_data;
}

void initialize(AgentAccess& agent_access, std::string_view const& data);
void initialize(Gender& gender, std::string_view const& data);

inline void initialize(RegionPositionLookAt& region_position_look_at, std::string_view const& data)
{
  region_position_look_at.assign_from_string(data);
}

inline void initialize(Position& position, std::string_view const& data)
{
  initialize(position.m_position, data);
}

inline void initialize(LookAt& look_at, std::string_view const& data)
{
  initialize(look_at.m_look_at, data);
}

void initialize(RegionHandle& region_handle, std::string_view const& data);

} // namespace xmlrpc
