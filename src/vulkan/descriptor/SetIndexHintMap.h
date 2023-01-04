#pragma once

#include "SetIndex.h"
#include "utils/AIAlert.h"
#include <map>
#include "debug.h"

namespace vulkan::descriptor {

class SetIndexHintMap
{
 private:
  using set_map_container_t = utils::Vector<SetIndex, SetIndexHint>;
  set_map_container_t m_set_index_map;

 public:
  void add_from_to(descriptor::SetIndexHint sih1, descriptor::SetIndexHint sih2)
  {
    DoutEntering(dc::vulkan, "add_from_to(" << sih1 << ", " << sih2);
    descriptor::SetIndex final_set_index{sih2.get_value()};
    if (sih1 >= m_set_index_map.iend())
      m_set_index_map.resize(sih1.get_value() + 1);
    // Don't call add_from_to twice for the same sih1.
    ASSERT(m_set_index_map[sih1].undefined());
    m_set_index_map[sih1] = final_set_index;
  }

  void clear()
  {
    m_set_index_map.clear();
  }

  descriptor::SetIndex convert(descriptor::SetIndexHint set_index_hint) const
  {
    // The value set_index_hint must have been passed to add_from_to as first argument, but it wasn't.
    //
    // That means that LogicalDevice::realize_pipeline_layout was called with as first argument a
    // realized_descriptor_set_layouts_w not pointing to vector containing a SetLayout object with
    // a m_set_index_hint member (larger or) equal to set_index_hint.
    //
    // That in turn means that PipelineFactory::push_back_descriptor_set_layout_binding, of the
    // PipelineFactory calling this function, was never called with set_index_hint as first argument.
    //
    // The reason for that should be because the glsl id string that belongs to set_index_hint
    // does not occur in the shader template code.
    if (set_index_hint >= m_set_index_map.iend())
      THROW_FALERT("SetIndexHint [HINT] is larger than the size of m_set_index_map ([SIZE]) allows!",
          AIArgs("[HINT]", set_index_hint)("[SIZE]", m_set_index_map.size()));
    return m_set_index_map[set_index_hint];
  }

  // Accessor.
  bool empty() const { return m_set_index_map.empty(); }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::descriptor
