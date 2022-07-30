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
