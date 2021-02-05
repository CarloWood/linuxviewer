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
};
