#include "sys.h"
#include "LocationContext.h"
#include "ShaderVariableAttribute.h"

namespace vulkan::shaderbuilder {

void LocationContext::update_location(ShaderVariableAttribute const* shader_variable_attribute)
{
  locations[shader_variable_attribute] = next_location;
  next_location += TypeInfo{shader_variable_attribute->m_glsl_type}.number_of_attribute_indices;
}

} // namespace vulkan::shaderbuilder
