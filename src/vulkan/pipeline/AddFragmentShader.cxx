#include "sys.h"
#include "AddFragmentShader.h"
#include "debug.h"

namespace vulkan::pipeline {

void AddFragmentShader::register_AddFragmentShader_with(task::PipelineFactory* pipeline_factory) const
{
  //FIXME implement
}

#ifdef CWDEBUG
void AddFragmentShader::print_on(std::ostream& os) const
{
  os << '{';
  os << '}';
}
#endif

} // namespace vulkan::pipeline
