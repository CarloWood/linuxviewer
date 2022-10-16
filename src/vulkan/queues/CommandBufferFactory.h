#pragma once

#include "ImmediateSubmitRequest.h"
#include "CommandBuffer.h"
#include "CommandPool.h"
#include "statefultask/ResourcePool.h"

namespace vulkan {

class CommandBufferFactory final : public statefultask::ResourceFactory
{
 public:
   static constexpr auto pool_type = vulkan::ImmediateSubmitRequest::pool_type;
   using resource_type = vulkan::handle::CommandBuffer;

 private:
  using command_pool_type = vulkan::CommandPool<pool_type>;
  command_pool_type m_command_pool;
#ifdef CWDEBUG
  mutable Ambifix m_ambifix;       // Ambifix set just before calling ResourcePool<CommandBufferFactory>::acquire, so it can be used in do_allocate.
#endif

 public:
  void set_ambifix(Ambifix ambifix) const { m_ambifix = std::move(ambifix); }

  // Implement virtual functions of statefultask::ResourceFactory.
  void do_allocate(void* ptr_to_array_of_resources, size_t size) override;
  void do_free(void const* ptr_to_array_of_resources, size_t size) override;

  CommandBufferFactory(vulkan::LogicalDevice const* logical_device, vulkan::QueueFamilyPropertiesIndex queue_family
      COMMA_CWDEBUG_ONLY(vulkan::Ambifix const& debug_name)) :
    m_command_pool(logical_device, queue_family
        COMMA_CWDEBUG_ONLY(debug_name)) { }

  command_pool_type const& command_pool() const { return m_command_pool; }
};

} // namespace vulkan
