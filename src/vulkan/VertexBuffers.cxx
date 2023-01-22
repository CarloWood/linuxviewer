#include "sys.h"
#include "VertexBuffers.h"
#include "SynchronousWindow.h"
#include "queues/CopyDataToBuffer.h"
#include "shader_builder/VertexShaderInputSet.h"
#include "debug.h"

namespace vulkan {

#ifdef CWDEBUG
void VertexBuffers::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_memory:" << m_memory <<
      ", m_binding_count:" << m_binding_count <<
      ", m_data:" << (void*)m_data;
  os << '}';
}
#endif

} // namespace vulkan
