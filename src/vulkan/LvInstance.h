#pragma once

#include "InstanceCreateInfo.h"

namespace vulkan {

class LvInstance
{
 protected:
  vk::Instance m_vulkan_instance;
  ~LvInstance();

  void createInstance_old();
};

} // namespace vulkan
