#include "sys.h"
#include "CommandBufferFactory.h"

namespace vulkan {

void CommandBufferFactory::do_allocate(void* ptr_to_array_of_resources, size_t size)
{
  // Allocate `size` command buffers from the command pool.
  m_command_pool.allocate_buffers(static_cast<uint32_t>(size), static_cast<resource_type*>(ptr_to_array_of_resources)
      COMMA_CWDEBUG_ONLY(m_ambifix));
}

void CommandBufferFactory::do_free(void const* ptr_to_array_of_resources, size_t size)
{
  m_command_pool.free_buffers(static_cast<uint32_t>(size), static_cast<resource_type const*>(ptr_to_array_of_resources));
}

} // namespace vulkan
