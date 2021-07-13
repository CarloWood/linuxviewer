#pragma once

#include "ApplicationCreateInfo.h"

// Extended initialization info with default values for HelloTriangleVulkanApplicationCreateInfo.
struct HelloTriangleVulkanApplicationCreateInfoExt
{
  int const version = 0;
};

// Initialization info for HelloTriangleVulkanApplication.
struct HelloTriangleVulkanApplicationCreateInfo : public ApplicationCreateInfo, public HelloTriangleVulkanApplicationCreateInfoExt
{
#ifdef CWDEBUG
  // Printing of this structure.
  void print_on(std::ostream& os) const;
#endif
};
