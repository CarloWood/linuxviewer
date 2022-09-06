#pragma once

#ifdef CWDEBUG

#include "utils/iomanip.h"

namespace vulkan {
class LogicalDevice;
} // namespace vulkan

namespace debug {

class set_device : public utils::iomanip::Sticky
{
 private:
  static utils::iomanip::Index s_index;

 public:
  set_device(vulkan::LogicalDevice const* pword_value) : Sticky(s_index, const_cast<vulkan::LogicalDevice*>(pword_value)) { }

  static vulkan::LogicalDevice const* get_pword_value(std::ostream& os)
  {
    return static_cast<vulkan::LogicalDevice const*>(get_pword_from(os, s_index));
  }
};

} // namespace debug

#endif // CWDEBUG
