#include "sys.h"
#include "VertexAttribute.h"
#include "debug.h"
#include <magic_enum.hpp>
#include <sstream>

namespace vulkan::shaderbuilder {

namespace {

#define AI_TYPE_CASE_RETURN(x) do { case Type::x: return #x; } while(0)

char const* type2name(Type glsl_type)
{
  switch (glsl_type)
  {
    case Type::Float: return "float";
    AI_TYPE_CASE_RETURN(vec2);
    AI_TYPE_CASE_RETURN(vec3);
    AI_TYPE_CASE_RETURN(vec4);
    AI_TYPE_CASE_RETURN(mat2);
    AI_TYPE_CASE_RETURN(mat3x2);
    AI_TYPE_CASE_RETURN(mat4x2);
    AI_TYPE_CASE_RETURN(mat2x3);
    AI_TYPE_CASE_RETURN(mat3);
    AI_TYPE_CASE_RETURN(mat4x3);
    AI_TYPE_CASE_RETURN(mat2x4);
    AI_TYPE_CASE_RETURN(mat3x4);
    AI_TYPE_CASE_RETURN(mat4);

    case Type::Double: return "double";
    AI_TYPE_CASE_RETURN(dvec2);
    AI_TYPE_CASE_RETURN(dvec3);
    AI_TYPE_CASE_RETURN(dvec4);
    AI_TYPE_CASE_RETURN(dmat2);
    AI_TYPE_CASE_RETURN(dmat3x2);
    AI_TYPE_CASE_RETURN(dmat4x2);
    AI_TYPE_CASE_RETURN(dmat2x3);
    AI_TYPE_CASE_RETURN(dmat3);
    AI_TYPE_CASE_RETURN(dmat4x3);
    AI_TYPE_CASE_RETURN(dmat2x4);
    AI_TYPE_CASE_RETURN(dmat3x4);
    AI_TYPE_CASE_RETURN(dmat4);

    case Type::Bool: return "bool";
    AI_TYPE_CASE_RETURN(bvec2);
    AI_TYPE_CASE_RETURN(bvec3);
    AI_TYPE_CASE_RETURN(bvec4);

    case Type::Int: return "int";
    AI_TYPE_CASE_RETURN(ivec2);
    AI_TYPE_CASE_RETURN(ivec3);
    AI_TYPE_CASE_RETURN(ivec4);

    case Type::Uint: return "uint";
    AI_TYPE_CASE_RETURN(uvec2);
    AI_TYPE_CASE_RETURN(uvec3);
    AI_TYPE_CASE_RETURN(uvec4);

    case Type::Int8: return "float";
    case Type::i8vec2: return "vec2";
    case Type::i8vec3: return "vec3";
    case Type::i8vec4: return "vec4";

    case Type::Uint8: return "float";
    case Type::u8vec2: return "vec2";
    case Type::u8vec3: return "vec3";
    case Type::u8vec4: return "vec4";

    case Type::Int16: return "float";
    case Type::i16vec2: return "vec2";
    case Type::i16vec3: return "vec3";
    case Type::i16vec4: return "vec4";

    case Type::Uint16: return "float";
    case Type::u16vec2: return "vec2";
    case Type::u16vec3: return "vec3";
    case Type::u16vec4: return "vec4";
  }
  AI_NEVER_REACHED
}

#undef AI_TYPE_CASE_RETURN

size_t type2size(Type glsl_type)
{
  return decode_rows(glsl_type) * decode_cols(glsl_type) * decode_typesize(glsl_type);
}

// The following format must be supported by Vulkan (so no test is necessary):
//
// VK_FORMAT_R8_UNORM
// VK_FORMAT_R8_SNORM
// VK_FORMAT_R8_UINT
// VK_FORMAT_R8_SINT
// VK_FORMAT_R8G8_UNORM
// VK_FORMAT_R8G8_SNORM
// VK_FORMAT_R8G8_UINT
// VK_FORMAT_R8G8_SINT
// VK_FORMAT_R8G8B8A8_UNORM
// VK_FORMAT_R8G8B8A8_SNORM
// VK_FORMAT_R8G8B8A8_UINT
// VK_FORMAT_R8G8B8A8_SINT
// VK_FORMAT_B8G8R8A8_UNORM
// VK_FORMAT_A8B8G8R8_UNORM_PACK32
// VK_FORMAT_A8B8G8R8_SNORM_PACK32
// VK_FORMAT_A8B8G8R8_UINT_PACK32
// VK_FORMAT_A8B8G8R8_SINT_PACK32
// VK_FORMAT_A2B10G10R10_UNORM_PACK32
// VK_FORMAT_R16_UNORM
// VK_FORMAT_R16_SNORM
// VK_FORMAT_R16_UINT
// VK_FORMAT_R16_SINT
// VK_FORMAT_R16_SFLOAT
// VK_FORMAT_R16G16_UNORM
// VK_FORMAT_R16G16_SNORM
// VK_FORMAT_R16G16_UINT
// VK_FORMAT_R16G16_SINT
// VK_FORMAT_R16G16_SFLOAT
// VK_FORMAT_R16G16B16A16_UNORM
// VK_FORMAT_R16G16B16A16_SNORM
// VK_FORMAT_R16G16B16A16_UINT
// VK_FORMAT_R16G16B16A16_SINT
// VK_FORMAT_R16G16B16A16_SFLOAT
// VK_FORMAT_R32_UINT
// VK_FORMAT_R32_SINT
// VK_FORMAT_R32_SFLOAT
// VK_FORMAT_R32G32_UINT
// VK_FORMAT_R32G32_SINT
// VK_FORMAT_R32G32_SFLOAT
// VK_FORMAT_R32G32B32_UINT
// VK_FORMAT_R32G32B32_SINT
// VK_FORMAT_R32G32B32_SFLOAT
// VK_FORMAT_R32G32B32A32_UINT
// VK_FORMAT_R32G32B32A32_SINT
// VK_FORMAT_R32G32B32A32_SFLOAT

vk::Format type2format(Type glsl_type)
{
  vk::Format format;
  int rows = decode_rows(glsl_type);
  switch (decode_typemask(glsl_type))
  {
    case float_mask:
      // 32_SFLOAT
      switch (rows)
      {
        case 1:
          format = vk::Format::eR32Sfloat;
          break;
        case 2:
          format = vk::Format::eR32G32Sfloat;
          break;
        case 3:
          format = vk::Format::eR32G32B32Sfloat;
          break;
        case 4:
          format = vk::Format::eR32G32B32A32Sfloat;
          break;
      }
      break;
    case double_mask:
      // 64_SFLOAT
      switch (rows)
      {
        case 1:
          format = vk::Format::eR64Sfloat;
          break;
        case 2:
          format = vk::Format::eR64G64Sfloat;
          break;
        case 3:
          format = vk::Format::eR64G64B64Sfloat;
          break;
        case 4:
          format = vk::Format::eR64G64B64A64Sfloat;
          break;
      }
      break;
    case bool_mask:
      // 8_UINT
      switch (rows)
      {
        case 1:
          format = vk::Format::eR8Uint;
          break;
        case 2:
          format = vk::Format::eR8G8Uint;
          break;
        case 3:
          format = vk::Format::eR8G8B8Uint;
          break;
        case 4:
          format = vk::Format::eR8G8B8A8Uint;
          break;
      }
      break;
    case int32_mask:
      // 32_SINT
      switch (rows)
      {
        case 1:
          format = vk::Format::eR32Sint;
          break;
        case 2:
          format = vk::Format::eR32G32Sint;
          break;
        case 3:
          format = vk::Format::eR32G32B32Sint;
          break;
        case 4:
          format = vk::Format::eR32G32B32A32Sint;
          break;
      }
      break;
    case uint32_mask:
      // 32_UINT
      switch (rows)
      {
        case 1:
          format = vk::Format::eR32Uint;
          break;
        case 2:
          format = vk::Format::eR32G32Uint;
          break;
        case 3:
          format = vk::Format::eR32G32B32Uint;
          break;
        case 4:
          format = vk::Format::eR32G32B32A32Uint;
          break;
      }
      break;
    case snorm8_mask:
      // int8_t
      switch (rows)
      {
        case 1:
          format = vk::Format::eR8Snorm;
          break;
        case 2:
          format = vk::Format::eR8G8Snorm;
          break;
        case 3:
          format = vk::Format::eR8G8B8Snorm;
          break;
        case 4:
          format = vk::Format::eR8G8B8A8Snorm;
          break;
      }
      break;
    case unorm8_mask:
      // uint8_t
      switch (rows)
      {
        case 1:
          format = vk::Format::eR8Unorm;
          break;
        case 2:
          format = vk::Format::eR8G8Unorm;
          break;
        case 3:
          format = vk::Format::eR8G8B8Unorm;
          break;
        case 4:
          format = vk::Format::eR8G8B8A8Unorm;
          break;
      }
      break;
    case snorm16_mask:
      // int16_t
      switch (rows)
      {
        case 1:
          format = vk::Format::eR16Snorm;
          break;
        case 2:
          format = vk::Format::eR16G16Snorm;
          break;
        case 3:
          format = vk::Format::eR16G16B16Snorm;
          break;
        case 4:
          format = vk::Format::eR16G16B16A16Snorm;
          break;
      }
      break;
    case unorm16_mask:
      // uint16_t
      switch (rows)
      {
        case 1:
          format = vk::Format::eR16Unorm;
          break;
        case 2:
          format = vk::Format::eR16G16Unorm;
          break;
        case 3:
          format = vk::Format::eR16G16B16Unorm;
          break;
        case 4:
          format = vk::Format::eR16G16B16A16Unorm;
          break;
      }
      break;
  }
  return format;
}

} // namespace

TypeInfo::TypeInfo(Type glsl_type) : name(type2name(glsl_type)), size(type2size(glsl_type)), number_of_attribute_indices((size - 1) / 16 + 1), format(type2format(glsl_type))
{
}

void LocationContext::update_location(VertexAttribute const* vertex_attribute)
{
  locations[vertex_attribute] = next_location;
  next_location += TypeInfo{vertex_attribute->m_glsl_type}.number_of_attribute_indices;
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
  TypeInfo glsl_type_info(m_glsl_type);
  oss << "layout(location = " << context.next_location << ") in " << glsl_type_info.name << ' ' << name() << ";\t// " << m_glsl_id_str << "\n";
  context.update_location(this);
  return oss.str();
}

#ifdef CWDEBUG
void VertexAttribute::print_on(std::ostream& os) const
{
  using namespace magic_enum::ostream_operators;

  os << '{';

  os << "m_glsl_type:" << m_glsl_type <<
      ", m_glsl_id_str:" << NAMESPACE_DEBUG::print_string(m_glsl_id_str) <<
      ", m_offset:" << m_offset;

  os << '}';
}
#endif

} // namespace vulkan::shaderbuilder
