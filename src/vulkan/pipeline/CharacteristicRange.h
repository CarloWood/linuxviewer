#ifndef VULKAN_PIPELINE_CHARACTERISTIC_RANGE_H
#define VULKAN_PIPELINE_CHARACTERISTIC_RANGE_H

#include "FlatCreateInfo.h"
#include "ShaderInputData.h"
#include "utils/AIRefCount.h"
#include "utils/Vector.h"
#include <iosfwd>
#include <bit>

namespace task {
class SynchronousWindow;
class PipelineFactory;
} // namespace task

namespace vulkan::pipeline {

struct IndexCategory;
using Index = utils::VectorIndex<IndexCategory>;

class CharacteristicRange : public AIRefCount
{
 public:
  using index_type = int;                                       // An index into the range that uniquely defines the value of the characteristic.

 protected:
  ShaderInputData& shader_input_data();

 public: // Required for Window::create_vertex_buffers ?!
  ShaderInputData const& shader_input_data() const;

 private:
  index_type const m_begin;
  index_type const m_end;
  int const m_range_width;
  task::PipelineFactory* m_owning_factory;

 public:
  // The default has a range of a single entry with index 0.
  CharacteristicRange(index_type begin = 0, index_type end = 1) : m_begin(begin), m_end(end),
    // Calculate the number of bits needed for a hash value of all possible values in the range [begin, end>.
    //
    // For example, if begin = 3 and end = 11, then the following values are used:
    //
    // 3  000
    // 4  001
    // 5  010
    // 6  011
    // 7  100
    // 8  101
    // 9  110
    // 10 111
    //
    // Hence, we need the bit_width of 7 = 11 - 3 - 1.
    //
    // Also note that if end = begin + 1, so that there is only a single index value (0),
    // then we calculate std::bit_width(0) = 0. That works because if the value is always
    // the same then we don't need to reserve any hash bits for it.
    m_range_width(std::bit_width(static_cast<unsigned int>(end - begin - 1)))
  {
    // end is not included in the range. It must always be larger than begin.
    ASSERT(end > begin);
  }

  void set_owner(task::PipelineFactory* owning_factory) { m_owning_factory = owning_factory; }

  // Accessors.
  index_type ibegin() const { return m_begin; }
  index_type iend() const { return m_end; }

  virtual void initialize(FlatCreateInfo& flat_create_info, task::SynchronousWindow const* owning_window) = 0;
  virtual void fill(FlatCreateInfo& flat_create_info, index_type index) const = 0;

  // An Index is constructed by setting it to zero and then calling this function
  // for each CharacteristicRange that was added to a PipelineFactory with the current
  // characteristic index. This must be done in the same order as the characteristics
  // where added to the factory.
  void update(Index& pipeline_index, int index) const
  {
    // Out of range.
    ASSERT(m_begin <= index && index < m_end);
    pipeline_index <<= m_range_width;
    pipeline_index |= Index{static_cast<unsigned int>(index - m_begin)};
  }

  // Accessor.
  task::PipelineFactory* owning_factory(utils::Badge<ShaderInputData>) const { return m_owning_factory; }

#ifdef CWDEBUG
  virtual void print_on(std::ostream& os) const = 0;
#endif
};

// Index used for pipeline characteristic ranges.
using CharacteristicRangeIndex = utils::VectorIndex<CharacteristicRange>;

class Characteristic : public CharacteristicRange
{
  // Make these private. You shouldn't try to use them because they are just always 0 and 1.
  using CharacteristicRange::ibegin;
  using CharacteristicRange::iend;

  // When it is not a range - do everything in initialize.
  void fill(FlatCreateInfo& flat_create_info, index_type index) const override final { }
};

} // namespace vulkan::pipeline

#endif // VULKAN_PIPELINE_CHARACTERISTIC_RANGE_H
