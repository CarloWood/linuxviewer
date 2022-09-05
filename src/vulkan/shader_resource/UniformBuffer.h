#pragma once

#include "FrameResourceIndex.h"
#include "descriptor/SetKey.h"
#include "descriptor/SetKeyContext.h"
#include "memory/UniformBuffer.h"
#include "utils/Vector.h"
#ifdef CWDEBUG
#include "debug/vulkan_print_on.h"
#endif

namespace task {
class SynchronousWindow;
} // namespace vulkan

namespace vulkan::shader_resource {

// Represents the descriptor set of a uniform buffer.
class UniformBuffer
{
 private:
  descriptor::SetKey const m_descriptor_set_key;
  utils::Vector<memory::UniformBuffer, FrameResourceIndex> m_uniform_buffers;

 public:
  // Use create to initialize m_uniform_buffers.
  UniformBuffer() : m_descriptor_set_key(descriptor::SetKeyContext::instance()) { }

  void create(task::SynchronousWindow const* owning_window, vk::DeviceSize size
      COMMA_CWDEBUG_ONLY(Ambifix const& ambifix));

  memory::UniformBuffer& operator[](FrameResourceIndex frame_resource_index) { return m_uniform_buffers[frame_resource_index]; }
  memory::UniformBuffer const& operator[](FrameResourceIndex frame_resource_index) const { return m_uniform_buffers[frame_resource_index]; }

  descriptor::SetKey descriptor_set_key() const { return m_descriptor_set_key; }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::shader_resource
