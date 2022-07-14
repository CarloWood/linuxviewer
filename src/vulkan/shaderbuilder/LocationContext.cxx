#include "sys.h"
#include "LocationContext.h"
#include "ShaderVariableLayout.h"

namespace vulkan::shaderbuilder {

void LocationContext::update_location(ShaderVariableLayout const* shader_variable_layout)
{
  locations[shader_variable_layout] = next_location;
  next_location += TypeInfo{shader_variable_layout->m_glsl_type}.number_of_attribute_indices;
}

} // namespace vulkan::shaderbuilder
