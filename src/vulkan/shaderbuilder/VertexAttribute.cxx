#include "sys.h"
#include "VertexAttribute.h"
#include "debug.h"
#include <magic_enum.hpp>
#include <sstream>

namespace vulkan::shaderbuilder {

TypeInfo::TypeInfo(Type type)
{
  switch (type)
  {
    case Type::Bool:
      name = "bool";
      size = sizeof(bool);
      break;
    case Type::Int:
      name = "int";
      size = sizeof(int32_t);
      break;
    case Type::Uint:
      name = "uint";
      size = sizeof(uint32_t);
      break;
    case Type::Float:
      name = "float";
      size = sizeof(float);
      break;
    case Type::Double:
      name = "double";
      size = sizeof(double);
      break;
    case Type::bvec2:
      name = "bvec2";
      size = 2 * sizeof(bool);
      break;
    case Type::bvec3:
      name = "bvec3";
      size = 3 * sizeof(bool);
      break;
    case Type::bvec4:
      name = "bvec4";
      size = 4 * sizeof(bool);
      break;
    case Type::ivec2:
      name = "ivec2";
      size = 2 * sizeof(int32_t);
      break;
    case Type::ivec3:
      name = "ivec3";
      size = 3 * sizeof(int32_t);
      break;
    case Type::ivec4:
      name = "ivec4";
      size = 4 * sizeof(int32_t);
      break;
    case Type::uvec2:
      name = "uvec2";
      size = 2 * sizeof(uint32_t);
      break;
    case Type::uvec3:
      name = "uvec3";
      size = 3 * sizeof(uint32_t);
      break;
    case Type::uvec4:
      name = "uvec4";
      size = 4 * sizeof(uint32_t);
      break;
    case Type::vec2:
      name = "vec2";
      size = 2 * sizeof(float);
      break;
    case Type::vec3:
      name = "vec3";
      size = 3 * sizeof(float);
      break;
    case Type::vec4:
      name = "vec4";
      size = 4 * sizeof(float);
      break;
    case Type::dvec2:
      name = "dvec2";
      size = 2 * sizeof(double);
      break;
    case Type::dvec3:
      name = "dvec3";
      size = 3 * sizeof(double);
      break;
    case Type::dvec4:
      name = "dvec4";
      size = 4 * sizeof(double);
      break;
    case Type::mat2:
      name = "mat2";
      size = 4 * sizeof(float);
      break;
    case Type::mat2x3:
      name = "mat2x3";
      size = 6 * sizeof(float);
      break;
    case Type::mat2x4:
      name = "mat2x4";
      size = 8 * sizeof(float);
      break;
    case Type::mat3:
      name = "mat3";
      size = 9 * sizeof(float);
      break;
    case Type::mat3x4:
      name = "mat3x4";
      size = 12 * sizeof(float);
      break;
    case Type::mat4:
      name = "mat4";
      size = 16 * sizeof(float);
      break;
    case Type::dmat2:
      name = "dmat2";
      size = 4 * sizeof(double);
      break;
    case Type::dmat2x3:
      name = "dmat2x3";
      size = 6 * sizeof(double);
      break;
    case Type::dmat2x4:
      name = "dmat2x4";
      size = 8 * sizeof(double);
      break;
    case Type::dmat3:
      name = "dmat3";
      size = 9 * sizeof(double);
      break;
    case Type::dmat3x4:
      name = "dmat3x4";
      size = 12 * sizeof(double);
      break;
    case Type::dmat4:
      name = "dmat4";
      size = 16 * sizeof(double);
      break;
  }
  number_of_attribute_indices = (size - 1) / 16 + 1;
}

void LocationContext::update_location(VertexAttribute const* vertex_attribute)
{
  locations[vertex_attribute] = next_location;
  next_location += TypeInfo{vertex_attribute->m_type}.number_of_attribute_indices;
}

std::string VertexAttribute::name() const
{
  std::ostringstream oss;
  oss << 'v' << std::hash<std::string>{}(m_glsl_id_str);
  return oss.str();
}

std::string VertexAttribute::declaration(LocationContext& context) const
{
  std::ostringstream oss;
  TypeInfo type_info(m_type);
  oss << "layout(location = " << context.next_location << ") in " << type_info.name << ' ' << name() << ";\t// " << m_glsl_id_str << "\n";
  context.update_location(this);
  return oss.str();
}

#ifdef CWDEBUG
void VertexAttribute::print_on(std::ostream& os) const
{
  using namespace magic_enum::ostream_operators;

  os << '{';

  os << "m_type:" << m_type <<
      ", m_glsl_id_str:" << NAMESPACE_DEBUG::print_string(m_glsl_id_str) <<
      ", m_offset:" << m_offset;

  os << '}';
}
#endif

} // namespace vulkan::shaderbuilder
