#ifndef VULKAN_PIPELINE_CHARACTERISTIC_RANGE_H
#define VULKAN_PIPELINE_CHARACTERISTIC_RANGE_H

#include "CharacteristicRangeBridge.h"
#include "shader_builder/ShaderIndex.h"
#include "utils/AIRefCount.h"
#include "utils/Vector.h"
#include "statefultask/AIStatefulTask.h"
#include "threadsafe/aithreadsafe.h"
#include <iosfwd>
#include <bit>

namespace vulkan {

namespace shader_builder {
class UniformBufferBase;
namespace shader_resource {
class CombinedImageSampler;
} // namespace shader_resource
} // namespace shader_builder

namespace descriptor {
class SetKeyPreference;
} // namespace descriptor

namespace task {
class CharacteristicRange;
class PipelineFactory;
class SynchronousWindow;
} // namespace task

namespace pipeline {
class FlatCreateInfo;
struct IndexCategory;
using Index = utils::VectorIndex<IndexCategory>;
using CharacteristicRangeIndex = utils::VectorIndex<task::CharacteristicRange>;
} // namespace pipeline

namespace task {

class CharacteristicRange : public AIStatefulTask, public virtual pipeline::CharacteristicRangeBridge
{
 public:
  static constexpr condition_type do_fill = 1;
  static constexpr condition_type do_preprocess = 2;
  static constexpr condition_type do_compile = 4;
  static constexpr condition_type do_build_shaders = 8;
  static constexpr condition_type do_terminate = 16;

  condition_type m_needs_signals{do_fill};      // The signals that are required by the derived class.

  using index_type = int;                       // An index into the range that uniquely defines the value of the characteristic.
  using pipeline_index_t = aithreadsafe::Wrapper<pipeline::Index, aithreadsafe::policy::Primitive<std::mutex>>;

 protected:
  PipelineFactory* m_owning_factory;
  SynchronousWindow const* m_owning_window;
  pipeline::FlatCreateInfo* m_flat_create_info{};
  bool m_terminate{false};                      // Set to true when the task must terminate.
  pipeline::CharacteristicRangeIndex m_characteristic_range_index;

 private:
  index_type const m_begin;
  index_type const m_end;
  index_type m_fill_index{-1};

 public:
  // The default has a range of a single entry with index 0.
  CharacteristicRange(SynchronousWindow const* owning_window, index_type begin = 0, index_type end = 1 COMMA_CWDEBUG_ONLY(bool debug = false)) : AIStatefulTask(CWDEBUG_ONLY(debug)), m_owning_window(owning_window), m_begin(begin), m_end(end)
  {
    // end is not included in the range. It must always be larger than begin.
    ASSERT(end > begin);
  }

  // If the user Characteristic class is derived from AddShaderStage then we need to do preprocessing and compiling.
  void set_needs_signals() { if (get_add_shader_stage()) m_needs_signals |= do_preprocess|do_compile; }
  void register_with_the_flat_create_info() const
  {
    register_AddShaderStage_with(m_owning_factory, *this);
    register_AddVertexShader_with(m_owning_factory, *this);
    register_AddPushConstant_with(m_owning_factory, *this);
  }
  void set_owner(PipelineFactory* owning_factory) { m_owning_factory = owning_factory; }

  // Accessors.
  index_type ibegin() const { return m_begin; }
  index_type iend() const { return m_end; }
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
  unsigned int range_width() const { return std::bit_width(static_cast<unsigned int>(m_end - m_begin - 1)); }

  // An pipeline::Index is constructed by setting it to zero and then calling this function
  // for each CharacteristicRange that was added to a PipelineFactory with the current
  // characteristic index. This must be done in the same order as the characteristics
  // where added to the factory.
  //
  // Each characteristic has defined a range [m_begin, m_end>.
  // The 'pipeline_index' contains the values of each of its characteristics:
  //
  //                        <--------------range_shift---------------> (for range 10)
  // pipeline_index: 0010110101001101110100100010101010100011101010100
  //                   ^^^^^----^^^^^^----^^^^^^-------^^^^---^^----^^
  //                     /    /    \   \ ...                         \
  //                    /  range 9  8   7                            0
  //                   /
  //     characteristic range 10 offset (should be added to m_begin to get the (characteristic) index_type.
  //
  void update(pipeline_index_t::wat const& pipeline_index, size_t characteristic_range_index, int index, unsigned int range_shift) const
  {
    // Out of range.
    ASSERT(m_begin <= index && index < m_end);
    *pipeline_index |= pipeline::Index{static_cast<unsigned int>(index - m_begin) << range_shift};
  }

  // Accessor.
  pipeline::CharacteristicRangeIndex characteristic_range_index() const { return m_characteristic_range_index; }
  index_type fill_index() const { return m_fill_index; }
  condition_type needs_signals() const { return m_needs_signals; }
  void begin_new_pipeline() { reset_push_constant_ranges(); reset_vertex_shader_location_contex(); }

 protected:
  inline void add_combined_image_sampler(shader_builder::shader_resource::CombinedImageSampler const& combined_image_sampler,
      std::vector<descriptor::SetKeyPreference> const& preferred_descriptor_sets = {},
      std::vector<descriptor::SetKeyPreference> const& undesirable_descriptor_sets = {});

  inline void add_uniform_buffer(shader_builder::UniformBufferBase const& uniform_buffer,
      std::vector<descriptor::SetKeyPreference> const& preferred_descriptor_sets = {},
      std::vector<descriptor::SetKeyPreference> const& undesirable_descriptor_sets = {});

  //---------------------------------------------------------------------------
  // Task specific code
 protected:
  using direct_base_type = AIStatefulTask;
  state_type m_fill_state;                      // The state of the derived class that we should continue with when this
                                                // task is done with its current state.

  // The different states of the task.
  enum characteristic_range_state_type {
    CharacteristicRange_initialized = direct_base_type::state_end,
    CharacteristicRange_fill_or_terminate,
    CharacteristicRange_filled,
    CharacteristicRange_preprocess,
    CharacteristicRange_compile,
    CharacteristicRange_build_shaders,
    CharacteristicRange_compiled
  };

  void set_fill_state(state_type fill_state) { m_fill_state = fill_state; }

 public:
  static state_type constexpr state_end = CharacteristicRange_compiled + 1;

  void set_flat_create_info(pipeline::FlatCreateInfo* flat_create_info) { m_flat_create_info = flat_create_info; }
  bool set_fill_index(index_type fill_index) { bool changed = m_fill_index != fill_index; m_fill_index = fill_index; return changed; }
  void set_characteristic_range_index(pipeline::CharacteristicRangeIndex characteristic_range_index) { m_characteristic_range_index = characteristic_range_index; }
  void terminate() { m_terminate = true; signal(do_terminate); }

 protected:
  char const* task_name_impl() const override { return "CharacteristicRange"; }
  char const* state_str_impl(state_type run_state) const override;
  char const* condition_str_impl(condition_type condition) const override;
  void multiplex_impl(state_type run_state) override;
  void initialize_impl() override;

 private:
  // Override of CharacteristicRangeBridge.
  inline shader_builder::ShaderResourceDeclaration* realize_shader_resource_declaration(std::string glsl_id_full, vk::DescriptorType descriptor_type, shader_builder::ShaderResourceBase const& shader_resource, descriptor::SetIndexHint set_index_hint) final;

  PipelineFactory* get_owning_factory() const final { return m_owning_factory; }

#ifdef CWDEBUG
 public:
  virtual void print_on(std::ostream& os) const = 0;
#endif
};

} // namespace task
} // namespace vulkan

#endif // VULKAN_PIPELINE_CHARACTERISTIC_RANGE_H

#ifndef PIPELINE_PIPELINE_FACTORY_H
#include "PipelineFactory.h"
#endif

#ifndef VULKAN_PIPELINE_CHARACTERISTIC_RANGE_H_definitions
#define VULKAN_PIPELINE_CHARACTERISTIC_RANGE_H_definitions

namespace vulkan::task {

void CharacteristicRange::add_combined_image_sampler(shader_builder::shader_resource::CombinedImageSampler const& combined_image_sampler,
    std::vector<descriptor::SetKeyPreference> const& preferred_descriptor_sets,
    std::vector<descriptor::SetKeyPreference> const& undesirable_descriptor_sets)
{
  m_owning_factory->add_combined_image_sampler({},
      combined_image_sampler, this, preferred_descriptor_sets, undesirable_descriptor_sets);
}

void CharacteristicRange::add_uniform_buffer(shader_builder::UniformBufferBase const& uniform_buffer,
    std::vector<descriptor::SetKeyPreference> const& preferred_descriptor_sets,
    std::vector<descriptor::SetKeyPreference> const& undesirable_descriptor_sets)
{
  m_owning_factory->add_uniform_buffer({},
      uniform_buffer, this, preferred_descriptor_sets, undesirable_descriptor_sets);
}

shader_builder::ShaderResourceDeclaration* CharacteristicRange::realize_shader_resource_declaration(std::string glsl_id_full, vk::DescriptorType descriptor_type, shader_builder::ShaderResourceBase const& shader_resource, descriptor::SetIndexHint set_index_hint)
{
  DoutEntering(dc::setindexhint, "CharacteristicRange::realize_shader_resource_declaration(\"" << glsl_id_full << "\", " << descriptor_type << ", " << shader_resource << ", " << set_index_hint << ")");
  return m_owning_factory->realize_shader_resource_declaration({}, glsl_id_full, descriptor_type, shader_resource, set_index_hint);
}

} // namespace vulkan::task

#endif // VULKAN_PIPELINE_CHARACTERISTIC_RANGE_H_definitions
