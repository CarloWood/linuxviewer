#pragma once

#include "utils/Vector.h"
#include <boost/intrusive_ptr.hpp>
#include <iosfwd>
#include "debug.h"

namespace vulkan::task {
class PipelineFactory;
} // namespace vulkan::task

namespace vulkan::pipeline {

struct IndexCategory;
using Index = utils::VectorIndex<IndexCategory>;

struct Handle
{
  using PipelineFactoryIndex = utils::VectorIndex<boost::intrusive_ptr<task::PipelineFactory>>;

  PipelineFactoryIndex m_pipeline_factory_index;
  Index m_pipeline_index;

  operator bool() const { return !m_pipeline_index.undefined();  }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::pipeline
