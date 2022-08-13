#include "sys.h"

#include "VertexAttribute.h"
#include "pipeline/Pipeline.h"
#include <magic_enum.hpp>
#include <sstream>
#ifdef CWDEBUG
#include "vk_utils/PrintPointer.h"
#endif

namespace vulkan::shaderbuilder {

namespace {

std::string type2name(BasicType glsl_type)
{
  glsl::ScalarIndex scalar_type = glsl_type.scalar_type();
  int rows = glsl_type.rows();
  int cols = glsl_type.cols();
  glsl::Kind kind = glsl_type.kind();

  // Check out of range.
  ASSERT(0 <= scalar_type && scalar_type < glsl::number_of_scalar_types);
  ASSERT(1 <= rows && rows <= 4);
  ASSERT(1 <= cols && cols <= 4);
  // Row-vectors are not used in this rows/cols encoding.
  ASSERT(!(rows == 1 && cols > 1));
  // There are only matrices of float and double.
  ASSERT(kind != glsl::Matrix || (scalar_type == glsl::eFloat || scalar_type == glsl::eDouble));

  std::string type_name;
  switch (scalar_type)
  {
    case glsl::eDouble:
      type_name = 'd';
      break;
    case glsl::eBool:
      type_name = 'b';
      break;
    case glsl::eUint:
      type_name = 'u';
      break;
    case glsl::eInt:
      type_name = 'i';
      break;
    default:
      break;
  }
  switch (kind)
  {
    case glsl::Scalar:
      type_name = to_string(scalar_type);
      break;
    case glsl::Vector:
      type_name += "vec" + std::to_string(rows);
      break;
    case glsl::Matrix:
      type_name += "mat" + std::to_string(cols) + "x" + std::to_string(rows);
      break;
  }
  return type_name;
}

} // namespace

std::string VertexAttributeLayout::name() const
{
  std::ostringstream oss;
  oss << 'v' << std::hash<std::string>{}(m_glsl_id_str);
  return oss.str();
}

std::string VertexAttribute::declaration(pipeline::Pipeline* pipeline) const
{
  LocationContext& context = pipeline->vertex_shader_location_context();

  std::ostringstream oss;
  ASSERT(context.next_location <= 999); // 3 chars max.
  oss << "layout(location = " << context.next_location << ") in " << type2name(m_layout->m_base_type) << ' ' << m_layout->name();
  //      ^^^^^^^^^^^^^^^^^^                               ^^^^^                                          ^
  if (m_layout->m_array_size > 0)
    oss << '[' << m_layout->m_array_size << ']';
  //        ^     ^^^                        ^
  oss << ";\t// " << m_layout->m_glsl_id_str << "\n";
  //      ^^ ^^^     ^     30 chars.
  context.update_location(this);
  return oss.str();
}

#ifdef CWDEBUG
void BasicType::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_standard:" << to_string(standard()) <<
      ", m_rows:" << m_rows <<
      ", m_cols:" << m_cols <<
      ", m_scalar_type:" << scalar_type() <<
      ", m_log2_alignment:" << m_log2_alignment <<
      ", m_size:" << m_size <<
      ", m_array_stride:" << m_array_stride;
  os << '}';
}

void VertexAttributeLayout::print_on(std::ostream& os) const
{
  os << '{';

  os << "m_base_type:" << m_base_type <<
      ", m_glsl_id_str:" << NAMESPACE_DEBUG::print_string(m_glsl_id_str) <<
      ", m_offset:" << m_offset;
  if (m_array_size > 0)
    os << ", m_array_size:" << m_array_size;

  os << '}';
}

void VertexAttribute::print_on(std::ostream& os) const
{
  os << '{';

  os << "m_layout: " << vk_utils::print_pointer(m_layout) <<
      ", m_binding:" << m_binding;

  os << '}';
}
#endif

} // namespace vulkan::shaderbuilder
