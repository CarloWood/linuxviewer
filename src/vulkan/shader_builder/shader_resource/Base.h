#pragma once

#ifdef CWDEBUG
#include <iosfwd>
#endif

namespace vulkan::shader_builder::shader_resource {

// Base class for shader resources.
class Base
{
#ifdef CWDEBUG
 public:
  virtual void print_on(std::ostream& os) const = 0;
#endif
};

} // namespace vulkan::shader_builder::shader_resource
