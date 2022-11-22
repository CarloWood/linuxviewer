#ifndef VULKAN_PIPELINE_CHARACTERISTIC_RANGE_H
#define VULKAN_PIPELINE_CHARACTERISTIC_RANGE_H

#include "FlatCreateInfo.h"
#include "ShaderInputData.h"
#include "utils/AIRefCount.h"
#include "utils/Vector.h"
#include "statefultask/AIStatefulTask.h"
#include "threadsafe/aithreadsafe.h"
#include <iosfwd>
#include <bit>

namespace task {
class SynchronousWindow;
class PipelineFactory;
} // namespace task

namespace vulkan::pipeline {

struct IndexCategory;
using Index = utils::VectorIndex<IndexCategory>;

class CharacteristicRange : public AIStatefulTask
{
 public:
  static constexpr condition_type do_fill = 1;
  static constexpr condition_type do_compile = 2;
  static constexpr condition_type do_terminate = 4;

  using index_type = int;                       // An index into the range that uniquely defines the value of the characteristic.
  using pipeline_index_t = aithreadsafe::Wrapper<vulkan::pipeline::Index, aithreadsafe::policy::Primitive<std::mutex>>;

 protected:
  task::PipelineFactory* m_owning_factory;
  task::SynchronousWindow const* m_owning_window;
  FlatCreateInfo* m_flat_create_info{};
  descriptor::SetIndexHintMap const* m_set_index_hint_map{};
  bool m_terminate{false};                      // Set to true when the task must terminate.
  CharacteristicRangeIndex m_characteristic_range_index;

 private:
  index_type const m_begin;
  index_type const m_end;
  index_type m_fill_index{-1};
#ifdef CWDEBUG
  state_type m_expected_state{direct_base_type::state_end};
#endif

 public:
  // The default has a range of a single entry with index 0.
  CharacteristicRange(task::SynchronousWindow const* owning_window, index_type begin = 0, index_type end = 1 COMMA_CWDEBUG_ONLY(bool debug = false)) : AIStatefulTask(CWDEBUG_ONLY(debug)), m_owning_window(owning_window), m_begin(begin), m_end(end)
  {
    DoutEntering(dc::statefultask(mSMDebug), "CharacteristicRange() [" << (void*)this << "]");
    // end is not included in the range. It must always be larger than begin.
    ASSERT(end > begin);
  }

  void set_owner(task::PipelineFactory* owning_factory) { m_owning_factory = owning_factory; }
  void set_set_index_hint_map(descriptor::SetIndexHintMap const* set_index_hint_map) { m_set_index_hint_map = set_index_hint_map; }

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
    *pipeline_index |= Index{static_cast<unsigned int>(index - m_begin) << range_shift};
  }

  // Accessor.
  task::PipelineFactory* owning_factory(utils::Badge<ShaderInputData>) const { return m_owning_factory; }
  CharacteristicRangeIndex characteristic_range_index() const { return m_characteristic_range_index; }
  index_type fill_index() const { return m_fill_index; }

 protected:
  inline auto const& shader_stage_create_infos() const;
  inline auto const& sorted_descriptor_set_layouts() const;
  inline auto& sorted_descriptor_set_layouts();

  template<typename ENTRY>
  requires (std::same_as<typename shader_builder::ShaderVariableLayouts<ENTRY>::tag_type, glsl::per_vertex_data> ||
            std::same_as<typename shader_builder::ShaderVariableLayouts<ENTRY>::tag_type, glsl::per_instance_data>)
  inline void add_vertex_input_binding(shader_builder::VertexShaderInputSet<ENTRY>& vertex_shader_input_set);

  inline void add_combined_image_sampler(shader_builder::shader_resource::CombinedImageSampler const& combined_image_sampler,
      std::vector<descriptor::SetKeyPreference> const& preferred_descriptor_sets = {},
      std::vector<descriptor::SetKeyPreference> const& undesirable_descriptor_sets = {});

  inline void add_uniform_buffer(shader_builder::shader_resource::UniformBufferBase const& uniform_buffer,
      std::vector<descriptor::SetKeyPreference> const& preferred_descriptor_sets = {},
      std::vector<descriptor::SetKeyPreference> const& undesirable_descriptor_sets = {});

  template<typename ENTRY>
  requires (std::same_as<typename shader_builder::ShaderVariableLayouts<ENTRY>::tag_type, glsl::push_constant_std430>)
  inline void add_push_constant();

  inline void preprocess1(shader_builder::ShaderInfo const& shader_info);
  inline auto vertex_binding_descriptions() const;
  inline auto vertex_input_attribute_descriptions() const;
  inline void realize_descriptor_set_layouts(LogicalDevice const* logical_device);

  inline void build_shader(task::SynchronousWindow const* owning_window,
      shader_builder::ShaderIndex const& shader_index, shader_builder::ShaderCompiler const& compiler,
      shader_builder::SPIRVCache& spirv_cache, descriptor::SetIndexHintMap const* set_index_hint_map
      COMMA_CWDEBUG_ONLY(AmbifixOwner const& ambifix));

  inline void build_shader(task::SynchronousWindow const* owning_window,
      shader_builder::ShaderIndex const& shader_index, shader_builder::ShaderCompiler const& compiler,
      descriptor::SetIndexHintMap const* set_index_hint_map
      COMMA_CWDEBUG_ONLY(AmbifixOwner const& ambifix));

  inline auto push_constant_ranges() const;

 public:
  inline auto const& vertex_shader_input_sets() const;

  //---------------------------------------------------------------------------
  // Task specific code
 protected:
  using direct_base_type = AIStatefulTask;
  state_type m_continue_state;                  // The state of the derived class that we should continue with when this task is done with its current state.

  // The different states of the task.
  enum characteristic_range_state_type {
    CharacteristicRange_initialized = direct_base_type::state_end,
    CharacteristicRange_continue_or_terminate,
    CharacteristicRange_filled,
    CharacteristicRange_compiled
  };

  void set_continue_state(state_type continue_state)
  {
    m_continue_state = continue_state;
  }

 public:
  static state_type constexpr state_end = CharacteristicRange_compiled + 1;

  void set_flat_create_info(FlatCreateInfo* flat_create_info) { m_flat_create_info = flat_create_info; }
  void set_fill_index(index_type fill_index) { m_fill_index = fill_index; }
  void set_characteristic_range_index(CharacteristicRangeIndex characteristic_range_index) { m_characteristic_range_index = characteristic_range_index; }
  void terminate() { m_terminate = true; signal(do_terminate); }

 protected:
  char const* task_name_impl() const override { return "CharacteristicRange"; }
  char const* state_str_impl(state_type run_state) const override;
  char const* condition_str_impl(condition_type condition) const override;
  void multiplex_impl(state_type run_state) override;

#ifdef CWDEBUG
 public:
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

 private:
  // Hide CharacteristicRange_initialized.
  enum UseCharacteristic {
    CharacteristicRange_initialized,            // Don't use these, use Characteristic_*.
    CharacteristicRange_check_terminate,
    CharacteristicRange_filled
  };

 protected:
  using direct_base_type = vulkan::pipeline::CharacteristicRange;

  // The different states of this task.
  enum Characteristic_state_type {
    Characteristic_initialized = direct_base_type::state_end,
    Characteristic_fill,
    Characteristic_compile_or_terminate,
    Characteristic_compiled
  };

 public:
  static constexpr state_type state_end = Characteristic_compiled + 1;

  Characteristic(task::SynchronousWindow const* owning_window COMMA_CWDEBUG_ONLY(bool debug)) :
    CharacteristicRange(owning_window COMMA_CWDEBUG_ONLY(0, 1, debug)) { }

 protected:
  char const* state_str_impl(state_type run_state) const override;
  void multiplex_impl(state_type run_state) override;
};

} // namespace vulkan::pipeline

#endif // VULKAN_PIPELINE_CHARACTERISTIC_RANGE_H

#ifndef PIPELINE_PIPELINE_FACTORY_H
#include "PipelineFactory.h"
#endif

#ifndef VULKAN_PIPELINE_CHARACTERISTIC_RANGE_H_definitions
#define VULKAN_PIPELINE_CHARACTERISTIC_RANGE_H_definitions

namespace vulkan::pipeline {

// Inline proxy to ShaderInputData interface.
auto const& CharacteristicRange::vertex_shader_input_sets() const
{
  return m_owning_factory->shader_input_data({}).vertex_shader_input_sets({});
}

auto const& CharacteristicRange::shader_stage_create_infos() const {
  return m_owning_factory->shader_input_data({}).shader_stage_create_infos({});
}

auto const& CharacteristicRange::sorted_descriptor_set_layouts() const {
  return m_owning_factory->shader_input_data({}).sorted_descriptor_set_layouts({});
}

auto& CharacteristicRange::sorted_descriptor_set_layouts() {
  return m_owning_factory->shader_input_data({}).sorted_descriptor_set_layouts({});
}

template<typename ENTRY>
requires (std::same_as<typename shader_builder::ShaderVariableLayouts<ENTRY>::tag_type, glsl::per_vertex_data> ||
          std::same_as<typename shader_builder::ShaderVariableLayouts<ENTRY>::tag_type, glsl::per_instance_data>)
void CharacteristicRange::add_vertex_input_binding(shader_builder::VertexShaderInputSet<ENTRY>& vertex_shader_input_set)
{
  m_owning_factory->shader_input_data({}).add_vertex_input_binding({}, vertex_shader_input_set);
}

void CharacteristicRange::add_combined_image_sampler(shader_builder::shader_resource::CombinedImageSampler const& combined_image_sampler,
    std::vector<descriptor::SetKeyPreference> const& preferred_descriptor_sets,
    std::vector<descriptor::SetKeyPreference> const& undesirable_descriptor_sets)
{
  m_owning_factory->shader_input_data({}).add_combined_image_sampler({},
      combined_image_sampler, this, preferred_descriptor_sets, undesirable_descriptor_sets);
}

void CharacteristicRange::add_uniform_buffer(shader_builder::shader_resource::UniformBufferBase const& uniform_buffer,
    std::vector<descriptor::SetKeyPreference> const& preferred_descriptor_sets,
    std::vector<descriptor::SetKeyPreference> const& undesirable_descriptor_sets)
{
  m_owning_factory->shader_input_data({}).add_uniform_buffer({},
      uniform_buffer, this, preferred_descriptor_sets, undesirable_descriptor_sets);
}

template<typename ENTRY>
requires (std::same_as<typename shader_builder::ShaderVariableLayouts<ENTRY>::tag_type, glsl::push_constant_std430>)
void CharacteristicRange::add_push_constant()
{
  m_owning_factory->shader_input_data({}).add_push_constant<ENTRY>({});
}

void CharacteristicRange::preprocess1(shader_builder::ShaderInfo const& shader_info)
{
  m_owning_factory->shader_input_data({}).preprocess1({}, shader_info);
}

auto CharacteristicRange::vertex_binding_descriptions() const
{
  return m_owning_factory->shader_input_data({}).vertex_binding_descriptions({});
}

auto CharacteristicRange::vertex_input_attribute_descriptions() const
{
  return m_owning_factory->shader_input_data({}).vertex_input_attribute_descriptions({});
}

void CharacteristicRange::realize_descriptor_set_layouts(LogicalDevice const* logical_device)
{
  m_owning_factory->shader_input_data({}).realize_descriptor_set_layouts({}, logical_device);
}

void CharacteristicRange::build_shader(task::SynchronousWindow const* owning_window,
    shader_builder::ShaderIndex const& shader_index, shader_builder::ShaderCompiler const& compiler,
    shader_builder::SPIRVCache& spirv_cache, descriptor::SetIndexHintMap const* set_index_hint_map
    COMMA_CWDEBUG_ONLY(AmbifixOwner const& ambifix))
{
  m_owning_factory->shader_input_data({}).build_shader({},
      owning_window, shader_index, compiler, spirv_cache, set_index_hint_map
      COMMA_CWDEBUG_ONLY(ambifix));
}

void CharacteristicRange::build_shader(task::SynchronousWindow const* owning_window,
    shader_builder::ShaderIndex const& shader_index, shader_builder::ShaderCompiler const& compiler,
    descriptor::SetIndexHintMap const* set_index_hint_map
    COMMA_CWDEBUG_ONLY(AmbifixOwner const& ambifix))
{
  m_owning_factory->shader_input_data({}).build_shader({},
      owning_window, shader_index, compiler, set_index_hint_map
      COMMA_CWDEBUG_ONLY(ambifix));
}

auto CharacteristicRange::push_constant_ranges() const
{
  return m_owning_factory->shader_input_data({}).push_constant_ranges({});
}

} // namespace vulkan::pipeline

#endif // VULKAN_PIPELINE_CHARACTERISTIC_RANGE_H_definitions
