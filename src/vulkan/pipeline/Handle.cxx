#include "sys.h"
#include "Handle.h"

namespace vulkan::pipeline {

#ifdef CWDEBUG
void Handle::print_on(std::ostream& os) const
{
  os << "<Handle>";
}
#endif

} // namespace vulkan::pipeline
