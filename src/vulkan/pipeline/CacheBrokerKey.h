#pragma once

#include "CacheData.h"
#include "PipelineCache.h"
#include "statefultask/BrokerKey.h"
#include "utils/pointer_hash.h"
#include <farmhash.h>

namespace vulkan::pipeline {

class CacheBrokerKey : public statefultask::BrokerKey, public CacheData
{
 protected:
  uint64_t hash() const final
  {
    return utils::pointer_hash(m_logical_device, m_owning_factory);
  }

  void initialize(boost::intrusive_ptr<AIStatefulTask> task) const final
  {
    task::PipelineCache& pipeline_cache = static_cast<task::PipelineCache&>(*task);
    CacheData::initialize(pipeline_cache);
  }

  unique_ptr copy() const final
  {
    return unique_ptr(new CacheBrokerKey(*this));
  }

  bool equal_to_impl(BrokerKey const& other) const final
  {
    CacheData const& data = static_cast<CacheBrokerKey const&>(other);
    return CacheData::operator==(data);
  }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const final
  {
    CacheData::print_on(os);
  }

  friend std::ostream& operator<<(std::ostream& os, CacheBrokerKey const& cache_broker_key)
  {
    cache_broker_key.print_on(os);
    return os;
  }
#endif
};

} // namespace vulkan::pipeline
