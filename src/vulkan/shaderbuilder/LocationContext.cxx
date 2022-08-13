#include "sys.h"
#include "LocationContext.h"
#include "VertexAttribute.h"

namespace vulkan::shaderbuilder {

void LocationContext::update_location(VertexAttribute const* vertex_attribute)
{
  locations[vertex_attribute] = next_location;
  int number_of_attribute_indices = vertex_attribute->layout().m_base_type.consumed_locations();
  if (vertex_attribute->layout().m_array_size > 0)
    number_of_attribute_indices *= vertex_attribute->layout().m_array_size;
  next_location += number_of_attribute_indices;
}

} // namespace vulkan::shaderbuilder
