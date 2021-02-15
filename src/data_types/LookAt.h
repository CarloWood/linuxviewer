#pragma once

#include "Vector3d.h"

struct LookAt
{
  Vector3d m_look_at;

  void assign_from_xmlrpc_string(std::string_view const& data)
  {
    evio::protocol::xmlrpc::initialize(m_look_at, data);
  }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};
