#pragma once

#include <blaze/math/StaticVector.h>
#include "evio/protocol/xmlrpc/initialize.h"
#ifdef CWDEBUG
#include <iosfwd>
#endif

using Vector3d = blaze::StaticVector<float, 3UL>;

// Overwrite the default operator<< for StaticVector's.
std::ostream& operator<<(std::ostream& os, Vector3d const& vector3d);

namespace evio::protocol::xmlrpc {

// Specialise this template because we can't add an assign_from_xmlrpc_string to Vector3d.
template<>
void initialize(Vector3d& vec, std::string_view const& vector3d_data);

} // namespace evio::protocol::xmlrpc
