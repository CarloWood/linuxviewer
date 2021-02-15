#pragma once

#include "RegionHandle.h"
#include "Position.h"
#include "LookAt.h"
#include <string_view>

class RegionPositionLookAt
{
 private:
  RegionHandle m_region_handle;
  Position m_position;
  LookAt m_look_at;

 public:
  RegionPositionLookAt() = default;

  void assign_from_string(std::string_view data);

  void assign_from_xmlrpc_string(std::string_view const& data)
  {
    // No xml escaping is necessary.
    assign_from_string(data);
  }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};
