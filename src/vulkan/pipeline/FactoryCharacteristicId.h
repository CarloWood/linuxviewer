#pragma once

#include "ConsecutiveRange.h"
#include "utils/Vector.h"
#include <boost/intrusive_ptr.hpp>
#include "debug.h"

namespace task {
class PipelineFactory;
} // namespace task

namespace vulkan::pipeline {
class CharacteristicRange;

// A unique ID for a given pipeline factory / characteristic range pair.
//
// Returned by PipelineFactory::add_characteristic<>.
//
class FactoryCharacteristicId
{
 public:
  using PipelineFactoryIndex = utils::VectorIndex<boost::intrusive_ptr<task::PipelineFactory>>;
  using CharacteristicRangeIndex = utils::VectorIndex<CharacteristicRange>;

 private:
  PipelineFactoryIndex m_factory_index;
  CharacteristicRangeIndex m_range_index;
  int m_characteristic_range_size;

 public:
  // Construct an undefined FactoryCharacteristicId.
  FactoryCharacteristicId() = default;

  FactoryCharacteristicId(PipelineFactoryIndex factory_index, CharacteristicRangeIndex range_index, int characteristic_range_size) :
    m_factory_index(factory_index), m_range_index(range_index), m_characteristic_range_size(characteristic_range_size) { }

  FactoryCharacteristicId& operator=(FactoryCharacteristicId const& rhs) = default;

  // Accessors.
  PipelineFactoryIndex factory_index() const { return m_factory_index; }
  CharacteristicRangeIndex range_index() const { return m_range_index; }
  int characteristic_range_size() const { return m_characteristic_range_size; }
  ConsecutiveRange full_range() const { return {0, m_characteristic_range_size}; }

  friend bool operator<(FactoryCharacteristicId const& lhs, FactoryCharacteristicId const& rhs)
  {
    if (lhs.m_factory_index < rhs.m_factory_index)
      return true;
    if (rhs.m_factory_index < lhs.m_factory_index)
      return false;
    // Paranoia check. The same characteristic range index should be the same characteristic and hence have the same range size.
    ASSERT(lhs.m_range_index != rhs.m_range_index || lhs.m_characteristic_range_size == rhs.m_characteristic_range_size);
    return lhs.m_range_index < rhs.m_range_index;
  }

  friend bool operator==(FactoryCharacteristicId const& lhs, FactoryCharacteristicId const& rhs)
  {
    // Paranoia check. The same characteristic range index should be the same characteristic and hence have the same range size.
    ASSERT(lhs.m_range_index != rhs.m_range_index || lhs.m_characteristic_range_size == rhs.m_characteristic_range_size);
    return lhs.m_factory_index == rhs.m_factory_index && lhs.m_range_index == rhs.m_range_index;
  }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::pipeline
