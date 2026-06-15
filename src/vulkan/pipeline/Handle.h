#pragma once

#include "IndexCategory.h"
#include "PipelineFactoryCategory.h"
#include "utils/Vector.h"
#include <boost/intrusive_ptr.hpp>
#include <iosfwd>
#include "debug.h"

namespace vulkan::pipeline {

using Index = utils::VectorIndex<IndexCategory>;

struct Handle
{
  using PipelineFactoryIndex = utils::VectorIndex<task::PipelineFactoryCategory>;

  PipelineFactoryIndex m_pipeline_factory_index;
  Index m_pipeline_index;

  operator bool() const { return !m_pipeline_index.undefined();  }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::pipeline
