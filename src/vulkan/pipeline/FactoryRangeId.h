#pragma once

#include "utils/Vector.h"
#include <boost/intrusive_ptr.hpp>

namespace task {
class PipelineFactory;
} // namespace task

namespace vulkan::pipeline {
class CharacteristicRange;

// A unique ID for a given pipeline factory / characteristic range pair.
//
// Returned by PipelineFactory::add_characteristic<>.
//
class FactoryRangeId
{
 public:
  using PipelineFactoryIndex = utils::VectorIndex<boost::intrusive_ptr<task::PipelineFactory>>;
  using CharacteristicRangeIndex = utils::VectorIndex<CharacteristicRange>;

 private:
  PipelineFactoryIndex m_factory_index;
  CharacteristicRangeIndex m_range_index;
  int m_characteristic_range_size;

 public:
  // Construct an undefined FactoryRangeId.
  FactoryRangeId() = default;

  FactoryRangeId(PipelineFactoryIndex factory_index, CharacteristicRangeIndex range_index, int characteristic_range_size) :
    m_factory_index(factory_index), m_range_index(range_index), m_characteristic_range_size(characteristic_range_size) { }

  FactoryRangeId& operator=(FactoryRangeId const& rhs) = default;

  // Accessors.
  PipelineFactoryIndex factory_index() const { return m_factory_index; }
  CharacteristicRangeIndex range_index() const { return m_range_index; }
  int characteristic_range_size() const { return m_characteristic_range_size; }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::pipeline
