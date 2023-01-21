#include "sys.h"
#include "AddFragmentShader.h"
#include "debug.h"

namespace vulkan::pipeline {

void AddFragmentShader::register_AddFragmentShader_with(task::PipelineFactory* pipeline_factory) const
{
  // Register vectors here of they're ever added.
  // FIXME: does AddFragmentShader every have such vectors? And if not, shouldn't AddFragmentShader be removed?
  //pipeline_factory->add_to_flat_create_info(...);
}

#ifdef CWDEBUG
void AddFragmentShader::print_on(std::ostream& os) const
{
  os << '{';
  os << '}';
}
#endif

} // namespace vulkan::pipeline
