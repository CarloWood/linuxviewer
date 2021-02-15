#pragma once

#include "Vector3d.h"

struct Position
{
  Vector3d m_position;

  void assign_from_xmlrpc_string(std::string_view const& data)
  {
    evio::protocol::xmlrpc::initialize(m_position, data);
  }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};
