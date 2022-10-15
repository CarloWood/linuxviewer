#include "sys.h"
#include "DeclarationsString.h"
#include "BasicType.h"
#include "math/glsl.h"
#include <sstream>

namespace vulkan::shader_builder {

void DeclarationsString::write_members_to(std::ostream& os, ShaderResourceMember::container_t const& members)
{
  for (ShaderResourceMember const& member : members)
  {
    if (!member.is_struct())
    {
      BasicType basic_type = member.basic_type();
      os << "  " << glsl::type2name(basic_type.scalar_type(), basic_type.rows(), basic_type.cols());
    }
    else
    {
      std::string const struct_name{member.struct_name()};
      if (!m_generated_structs.contains(struct_name))
      {
        std::ostringstream oss;
        oss << "struct " << struct_name << "\n{\n";
        write_members_to(oss, member.struct_type());
        oss << "};\n";
        m_content += oss.str();
        m_generated_structs.insert(struct_name);
      }
      os << "  " << struct_name;
    }
    os << ' ' << member.member_name();
    if (member.is_array())
      os << "[" << member.array_size() << "]";
    os << ";\n";
  }
}

} // namespace vulkan::shader_builder
