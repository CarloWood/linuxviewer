#pragma once

#include "debug.h"

namespace vulkan {

class ImGui {
 public:
  ImGui()
  {
    DoutEntering(dc::vulkan, "ImGui::ImGui()");
  }

  ~ImGui()
  {
    DoutEntering(dc::vulkan, "ImGui::~ImGui()");
  }
};

} // namespace vulkan
