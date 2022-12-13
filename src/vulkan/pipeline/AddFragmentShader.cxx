#include "sys.h"
#include "AddFragmentShader.h"
#include "debug.h"

namespace vulkan::pipeline {

#ifdef CWDEBUG
void AddFragmentShader::print_on(std::ostream& os) const
{
  os << '{';
  os << '}';
}
#endif

} // namespace vulkan::pipeline
