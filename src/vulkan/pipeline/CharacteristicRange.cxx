#include "sys.h"
#include "CharacteristicRange.h"
#include "PipelineFactory.h"

namespace vulkan::pipeline {

ShaderInputData& CharacteristicRange::shader_input_data() { return m_owning_factory->shader_input_data({}); }
ShaderInputData const& CharacteristicRange::shader_input_data() const { return m_owning_factory->shader_input_data({}); }

} // namespace vulkan::pipeline
