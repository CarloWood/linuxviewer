#pragma once

#include "debug.h"
#include <iosfwd>

namespace task {
class PipelineCache;
class PipelineFactory;
} // namespace task

namespace vulkan {

class LogicalDevice;

namespace pipeline {

class CacheData
{
 protected:
  LogicalDevice const* m_logical_device = {};           // A pipeline cache must be unique -at least- per logical device.
                                                        // You can not use a pipeline cache created for one logical device with another logical device.
  task::PipelineFactory const* m_owning_factory = {};   // We have one pipeline cache per factory - or each factory would still be
                                                        // slowed down as a result of concurrent accesses to the cache.
  CacheData() = default;

  bool operator==(CacheData const& other) const
  {
    // These should already be set.
    ASSERT(m_logical_device != nullptr && m_owning_factory != nullptr);
    return  m_logical_device == other.m_logical_device && m_owning_factory == other.m_owning_factory;
  }

 public:
  void set_logical_device(LogicalDevice const* logical_device)
  {
    DoutEntering(dc::vulkan, "vulkan::pipeline::CacheData::set_logical_device(" << logical_device << ")");
    m_logical_device = logical_device;
  }

  void set_owning_factory(task::PipelineFactory const* owning_factory)
  {
    DoutEntering(dc::vulkan, "vulkan::pipeline::CacheData::set_owning_factory(" << owning_factory << ")");
    m_owning_factory = owning_factory;
  }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace pipeline
} // namespace vulkan
