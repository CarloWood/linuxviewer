#pragma once

#include "data_types/BinaryData.h"
#include "data_types/DateTime.h"
#include "data_types/URI.h"
#include "data_types/UUID.h"
#include "data_types/Vector3d.h"
#include <boost/archive/iterators/xml_unescape.hpp>

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

} // namespace xmlrpc
