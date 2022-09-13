#include "sys.h"
#include "ShaderResource.h"
#include "pipeline/ShaderInputData.h"
#include "vk_utils/print_flags.h"
#include "vk_utils/PrintPointer.h"
#include <sstream>
#include "debug.h"

namespace vulkan::shader_builder {

#ifdef CWDEBUG
void ShaderResource::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_glsl_id:\"" << m_glsl_id <<
    "\", m_descriptor_type:" << m_descriptor_type <<
      ", m_set_index_hint:" << m_set_index_hint <<
      ", m_members_ptr:" << m_members_ptr;
  os << '}';
}
#endif

} // namespace vulkan::shader_builder
