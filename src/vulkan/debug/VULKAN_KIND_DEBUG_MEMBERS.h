#pragma once

#ifndef CWDEBUG
#define VULKAN_KIND_DEBUG_MEMBERS
#else

#include <iostream>

#define VULKAN_KIND_PRINT_ON_MEMBERS \
  void print_on(std::ostream& os) const \
  { \
    os.put('{'); \
    print_members(os); \
    os.put('}'); \
  } \
  void print_members(std::ostream& os) const;

#define VULKAN_KIND_DEBUG_MEMBERS VULKAN_KIND_PRINT_ON_MEMBERS

#endif // CWDEBUG
