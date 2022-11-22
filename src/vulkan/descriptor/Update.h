#pragma once

#include "utils/AIRefCount.h"
#ifdef CWDEBUG
#include <iosfwd>
#endif

namespace vulkan::descriptor {

class Update : public AIRefCount
{
 public:
  virtual bool is_needs_update() const = 0;

#ifdef CWDEBUG
  virtual void print_on(std::ostream& os) const = 0;
#endif
};

} // namespace vulkan::descriptor
