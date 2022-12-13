#pragma once

#include "AddShaderStage.h"
#include "debug.h"

namespace vulkan::pipeline {

class AddFragmentShader : public virtual AddShaderStage
{
 private:

 public:
  AddFragmentShader() = default;

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::pipeline
