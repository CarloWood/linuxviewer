#include "sys.h"
#include "LocationContext.h"
#include "VertexAttribute.h"

namespace vulkan::shaderbuilder {

void LocationContext::update_location(VertexAttribute const* vertex_attribute)
{
  locations[vertex_attribute] = next_location;
  next_location += TypeInfo{vertex_attribute->m_base_type}.number_of_attribute_indices;
}

} // namespace vulkan::shaderbuilder
