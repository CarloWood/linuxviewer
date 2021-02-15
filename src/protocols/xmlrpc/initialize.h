#pragma once

#include "data_types/Vector3d.h"
#include "data_types/URI.h"
#include "data_types/UUID.h"
#include "data_types/RegionPositionLookAt.h"
#include <boost/archive/iterators/xml_unescape.hpp>
#include <string_view>

class AgentAccess;
class Gender;
class RegionHandle;

namespace xmlrpc {

void initialize(AgentAccess& agent_access, std::string_view const& data);
void initialize(Gender& gender, std::string_view const& data);
void initialize(RegionHandle& region_handle, std::string_view const& data);
void initialize(Vector3d& vec, std::string_view const& vector3d_data);

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

} // namespace xmlrpc
