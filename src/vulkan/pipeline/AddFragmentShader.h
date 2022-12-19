#pragma once

#include "AddShaderStage.h"
#include "debug.h"

namespace vulkan::pipeline {

class AddFragmentShader : public virtual AddShaderStage
{
 private:
  void register_AddFragmentShader_with(task::PipelineFactory* pipeline_factory) const final;

 public:
  AddFragmentShader() = default;

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::pipeline
