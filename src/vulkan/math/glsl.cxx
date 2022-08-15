#include "sys.h"
#include "glsl.h"
#include "utils/AIAlert.h"
#include <string>
#include "debug.h"

namespace glsl {

std::string to_string(ScalarIndex scalar_type)
{
  switch (scalar_type)
  {
    case eFloat:
      return "float";
    case eDouble:
      return "double";
    case eBool:
      return "bool";
    case eInt:
      return "int";
    case eUint:
      return "uint";
    default:
      THROW_ALERT("glsl::to_string called for extended type [INDEX].", AIArgs("[INDEX]", (int)scalar_type));
  }
  AI_NEVER_REACHED
}

std::string type2name(glsl::ScalarIndex scalar_type, int rows, int cols)
{
  glsl::Kind const kind = get_kind(rows, cols);

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

#ifdef CWDEBUG
std::string to_string(Standard standard)
{
  switch (standard)
  {
    case vertex_attributes:
      return "vertex_attributes";
    case std140:
      return "std140";
    case std430:
      return "std430";
    case scalar:
      return "scalar";
  }
  AI_NEVER_REACHED
}

std::ostream& operator<<(std::ostream& os, ScalarIndex scalar_type)
{
  switch (scalar_type)
  {
    case eFloat:
      os << "eFloat";
      break;
    case eDouble:
      os << "eDouble";
      break;
    case eBool:
      os << "eBool";
      break;
    case eInt:
      os << "eInt";
      break;
    case eUint:
      os << "eUint";
      break;
    case eInt8:
      os << "eInt8";
      break;
    case eUint8:
      os << "eUint8";
      break;
    case eInt16:
      os << "eInt16";
      break;
    case eUint16:
      os << "eUint16";
      break;
  }
  return os;
}
#endif

} // namespace glsl
