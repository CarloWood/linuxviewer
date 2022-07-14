#include "sys.h"

#include "VertexAttributeEntry.h"
#include "pipeline/Pipeline.h"
#include <string>

namespace vulkan::shaderbuilder {

//static
std::string VertexAttributeEntry::declaration(ShaderVariableAttribute const* base, pipeline::Pipeline* pipeline)
{
  VertexAttributeEntry const* self = static_cast<VertexAttributeEntry const*>(base);
  LocationContext& context = pipeline->vertex_shader_location_context();

  std::ostringstream oss;
  TypeInfo glsl_type_info(self->m_glsl_type);
  ASSERT(context.next_location <= 999); // 3 chars max.
  oss << "layout(location = " << context.next_location << ") in " << glsl_type_info.name << ' ' << self->name() << ";\t// " << self->m_glsl_id_str << "\n";
  //      ^^^^^^^^^^^^^^^^^^                               ^^^^^                             ^                ^^ ^^^                       ^     30 chars.
  context.update_location(self);
  return oss.str();
}

} // namespace vulkan::shaderbuilder
