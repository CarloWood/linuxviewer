#include "sys.h"
#include "PipelineFactory.h"
#include "PipelineCache.h"
#include "Handle.h"
#include "SynchronousWindow.h"
#include "SynchronousTask.h"
#include "AddShaderStage.h"
#include "partitions/PartitionTask.h"
#include "partitions/ElementPair.h"
#include "partitions/PartitionIteratorExplode.h"
#include "shader_builder/shader_resource/CombinedImageSampler.h"
#include "shader_builder/shader_resource/UniformBuffer.h"
#include "vk_utils/TaskToTaskDeque.h"
#include "threadsafe/aithreadsafe.h"
#include "utils/at_scope_end.h"
#include "utils/almost_equal.h"

namespace task {
using namespace vulkan;
using namespace vulkan::pipeline;

namespace synchronous {

// Task used to synchronously move newly created pipelines to the SynchronousWindow.
class MoveNewPipelines final : public vk_utils::TaskToTaskDeque<SynchronousTask, std::pair<vulkan::Pipeline, vk::UniquePipeline>>
{
 private:
  SynchronousWindow::PipelineFactoryIndex m_factory_index;      // Index of the owning factory. There is a one-on-one relationship between PipelineFactory's and MoveNewPipelines's.

 protected:
  // The different states of the stateful task.
  enum move_new_pipelines_task_state_type {
    MoveNewPipelines_start = direct_base_type::state_end,
    MoveNewPipelines_need_action,
    MoveNewPipelines_done
  };

 public:
  // One beyond the largest state of this task.
  static constexpr state_type state_end = MoveNewPipelines_done + 1;

 public:
  MoveNewPipelines(SynchronousWindow* owning_window, SynchronousWindow::PipelineFactoryIndex factory_index COMMA_CWDEBUG_ONLY(bool debug)) :
    direct_base_type(owning_window COMMA_CWDEBUG_ONLY(debug)), m_factory_index(factory_index)
  {
    DoutEntering(dc::vulkan, "MoveNewPipelines::MoveNewPipelines(" << owning_window << ", " << factory_index << ") [" << this << "]")
  }

 protected:
  // Call finish() (or abort()), not delete.
  ~MoveNewPipelines() override
  {
    DoutEntering(dc::vulkan, "MoveNewPipelines::~MoveNewPipelines() [" << this << "]");
  }

  // Implementation of state_str for run states.
  char const* state_str_impl(state_type run_state) const override;
  char const* task_name_impl() const override;

  // Handle mRunState.
  void multiplex_impl(state_type run_state) override;

  // Program termination.
  void abort_impl() override;

  // If initialize_impl and/or finish_impl are added, then those must call SynchronousTask::initialize_impl and finish_impl.
};

char const* MoveNewPipelines::state_str_impl(state_type run_state) const
{
  switch (run_state)
  {
    AI_CASE_RETURN(MoveNewPipelines_start);
    AI_CASE_RETURN(MoveNewPipelines_need_action);
    AI_CASE_RETURN(MoveNewPipelines_done);
  }
  AI_NEVER_REACHED
}

char const* MoveNewPipelines::task_name_impl() const
{
  return "MoveNewPipelines";
}

void MoveNewPipelines::multiplex_impl(state_type run_state)
{
  switch (run_state)
  {
    case MoveNewPipelines_start:
      set_state(MoveNewPipelines_need_action);
      wait(need_action);
      break;
    case MoveNewPipelines_need_action:
      // Flush all newly created pipelines (if any) from the m_new_pipelines deque,
      // passing them one by one to SynchronousWindow::have_new_pipeline.
      flush_new_data([this](Datum&& datum){ owning_window()->have_new_pipeline(std::move(datum.first), std::move(datum.second)); });
      if (producer_not_finished())      // This calls wait(need_action) if not finished.
        break;
      set_state(MoveNewPipelines_done);
      [[fallthrough]];
    case MoveNewPipelines_done:
      owning_window()->pipeline_factory_done({}, m_factory_index);
      finish();
      break;
  }
}

void MoveNewPipelines::abort_impl()
{
  DoutEntering(dc::notice, "MoveNewPipelines::abort_impl()");
  flush_new_data([this](Datum&& datum){ Dout(dc::notice, "Still had {" << datum.first << ", " << *datum.second << "} in the deque."); });
  owning_window()->pipeline_factory_done({}, m_factory_index);
}

} // namespace synchronous

PipelineFactory::PipelineFactory(SynchronousWindow* owning_window, vulkan::Pipeline& pipeline_out, vk::RenderPass vh_render_pass
    COMMA_CWDEBUG_ONLY(bool debug)) : AIStatefulTask(CWDEBUG_ONLY(debug)),
    m_owning_window(owning_window), m_pipeline_out(pipeline_out), m_vh_render_pass(vh_render_pass), m_index(vulkan::Application::instance().m_dependent_tasks.add(this))
{
  DoutEntering(dc::statefultask(mSMDebug), "PipelineFactory::PipelineFactory(" << owning_window << ", @" << (void*)&pipeline_out << ", " << vh_render_pass << ")");
}

PipelineFactory::~PipelineFactory()
{
  DoutEntering(dc::statefultask(mSMDebug), "PipelineFactory::~PipelineFactory() [" << this << "]");

  vulkan::Application::instance().m_dependent_tasks.remove(m_index);
}

FactoryCharacteristicId PipelineFactory::add_characteristic(boost::intrusive_ptr<CharacteristicRange> characteristic_range)
{
  characteristic_range->set_owner(this);
  characteristic_range->register_with_the_flat_create_info();
  CharacteristicRangeIndex characteristic_range_index{m_characteristics.iend()};
  characteristic_range->set_characteristic_range_index(characteristic_range_index);
  int end = characteristic_range->iend();
  // Is this characteristic an AddVertexShader?
  AddShaderStage* add_shader_stage = dynamic_cast<AddShaderStage*>(characteristic_range.get());
  if (add_shader_stage != m_add_shader_stage)
  {
    // There can be only one (virtual) shader stage (characteristic) per pipeline.
    ASSERT(!m_add_shader_stage);
    m_add_shader_stage = add_shader_stage;
  }
  m_characteristics.push_back(std::move(characteristic_range));
  return FactoryCharacteristicId{ m_pipeline_factory_index, characteristic_range_index, end };
}

// Called from CharacteristicRange::add_combined_image_sampler which is
// Called from *UserCode*PipelineCharacteristic_initialize.
void PipelineFactory::add_combined_image_sampler(
    utils::Badge<CharacteristicRange>,
    shader_builder::shader_resource::CombinedImageSampler const& combined_image_sampler,
    CharacteristicRange const* adding_characteristic_range,
    std::vector<descriptor::SetKeyPreference> const& preferred_descriptor_sets,
    std::vector<descriptor::SetKeyPreference> const& undesirable_descriptor_sets)
{
  DoutEntering(dc::vulkan, "PipelineFactory::add_combined_image_sampler(" << combined_image_sampler << ", " << adding_characteristic_range << ", " <<
      preferred_descriptor_sets << ", " << undesirable_descriptor_sets << ") [" << this << "]");

  // Remember that this combined_image_sampler must be bound to its descriptor set from the PipelineFactory.
  descriptor::CombinedImageSamplerUpdater const* combined_image_sampler_task = combined_image_sampler.descriptor_task();
  register_shader_resource(combined_image_sampler_task, adding_characteristic_range, preferred_descriptor_sets, undesirable_descriptor_sets);
}

// Called from CharacteristicRange::add_uniform_buffer which is
// called from *UserCode*PipelineCharacteristic_initialize.
void PipelineFactory::add_uniform_buffer(
    utils::Badge<CharacteristicRange>,
    shader_builder::UniformBufferBase const& uniform_buffer,
    CharacteristicRange const* adding_characteristic_range,
    std::vector<descriptor::SetKeyPreference> const& preferred_descriptor_sets,
    std::vector<descriptor::SetKeyPreference> const& undesirable_descriptor_sets)
{
  DoutEntering(dc::vulkan, "PipelineFactory::add_uniform_buffer(" << uniform_buffer << ", " << preferred_descriptor_sets << ", " << undesirable_descriptor_sets << ") [" << this << "]");

  // Remember that this uniform buffer must be created from the PipelineFactory.
  register_shader_resource(&uniform_buffer, adding_characteristic_range, preferred_descriptor_sets, undesirable_descriptor_sets);
}

shader_builder::ShaderResourceDeclaration* PipelineFactory::realize_shader_resource_declaration(utils::Badge<vulkan::pipeline::CharacteristicRange>, std::string glsl_id_full, vk::DescriptorType descriptor_type, vulkan::shader_builder::ShaderResourceBase const& shader_resource, descriptor::SetIndexHint set_index_hint)
{
  DoutEntering(dc::vulkan, "PipelineFactory::realize_shader_resource_declaration(\"" << glsl_id_full << "\", " <<
      descriptor_type << ", " << shader_resource << set_index_hint << ")");

  glsl_id_to_shader_resource_t::wat glsl_id_to_shader_resource_w(m_glsl_id_to_shader_resource);
  shader_builder::ShaderResourceDeclaration shader_resource_tmp(glsl_id_full, descriptor_type, set_index_hint, shader_resource);
  auto ibp = glsl_id_to_shader_resource_w->insert(std::pair{glsl_id_full, shader_resource_tmp});
  // The m_glsl_id_full of each CombinedImageSampler must be unique. And of course, don't register the same shader_resource twice.
  ASSERT(ibp.second2());

  shader_builder::ShaderResourceDeclaration* shader_resource_ptr = &ibp.first->second;
  Dout(dc::vulkan, "Using ShaderResourceDeclaration* " << shader_resource_ptr);
  m_shader_resource_set_key_to_shader_resource_declaration.try_emplace_declaration(shader_resource.descriptor_set_key(), shader_resource_ptr);

  return shader_resource_ptr;
}

// Called from *UserCode*PipelineCharacteristic_initialize.
void PipelineFactory::realize_descriptor_set_layouts(utils::Badge<vulkan::pipeline::CharacteristicRange>, vulkan::LogicalDevice const* logical_device)
{
  DoutEntering(dc::shaderresource|dc::vulkan, "PipelineFactory::realize_descriptor_set_layouts(" << logical_device << ") [" << this << "]");
  sorted_descriptor_set_layouts_t::wat sorted_descriptor_set_layouts_w(m_sorted_descriptor_set_layouts);
#ifdef CWDEBUG
  Dout(dc::debug, "m_sorted_descriptor_set_layouts =");
  for (auto& descriptor_set_layout : *sorted_descriptor_set_layouts_w)
    Dout(dc::debug, "    " << descriptor_set_layout);
#endif
  for (auto& descriptor_set_layout : *sorted_descriptor_set_layouts_w)
    descriptor_set_layout.realize_handle(logical_device);
}

// Called from add_combined_image_sampler and add_uniform_buffer.
void PipelineFactory::register_shader_resource(
    shader_builder::ShaderResourceBase const* shader_resource,
    CharacteristicRange const* adding_characteristic_range,
    std::vector<descriptor::SetKeyPreference> const& preferred_descriptor_sets,
    std::vector<descriptor::SetKeyPreference> const& undesirable_descriptor_sets)
{
  DoutEntering(dc::vulkan, "PipelineFactory::register_shader_resource(" << vk_utils::print_pointer(shader_resource) << ", " << preferred_descriptor_sets << ", " <<
      undesirable_descriptor_sets << ") [" << this << "]");
  required_shader_resource_plus_characteristic_list_t::wat required_shader_resource_plus_characteristic_list_w(m_required_shader_resource_plus_characteristic_list);
  // Add a thread-safe pointer to the shader resource (ShaderResourceBase) to a list of required shader resources.
  // The shader_resource should point to a member of the Window class.
  required_shader_resource_plus_characteristic_list_w->emplace_back(ShaderResourcePlusCharacteristic{shader_resource, adding_characteristic_range, adding_characteristic_range->fill_index()}, preferred_descriptor_sets, undesirable_descriptor_sets);
}

char const* PipelineFactory::condition_str_impl(condition_type condition) const
{
  switch (condition)
  {
    AI_CASE_RETURN(pipeline_cache_set_up);
    AI_CASE_RETURN(fully_initialized);
    AI_CASE_RETURN(characteristics_initialized);
    AI_CASE_RETURN(characteristics_filled);
    AI_CASE_RETURN(characteristics_preprocessed);
    AI_CASE_RETURN(characteristics_compiled);
    AI_CASE_RETURN(obtained_create_lock);
    AI_CASE_RETURN(obtained_set_layout_binding_lock);
  }
  return direct_base_type::condition_str_impl(condition);
}

char const* PipelineFactory::state_str_impl(state_type run_state) const
{
  switch (run_state)
  {
    AI_CASE_RETURN(PipelineFactory_start);
    AI_CASE_RETURN(PipelineFactory_initialize);
    AI_CASE_RETURN(PipelineFactory_initialized);
    AI_CASE_RETURN(PipelineFactory_characteristics_initialized);
    AI_CASE_RETURN(PipelineFactory_top_multiloop_for_loop);
    AI_CASE_RETURN(PipelineFactory_top_multiloop_while_loop);
    AI_CASE_RETURN(PipelineFactory_characteristics_filled);
    AI_CASE_RETURN(PipelineFactory_characteristics_preprocessed);
    AI_CASE_RETURN(PipelineFactory_characteristics_compiled);
    AI_CASE_RETURN(PipelineFactory_create_shader_resources);
    AI_CASE_RETURN(PipelineFactory_initialize_shader_resources_per_set_index);
    AI_CASE_RETURN(PipelineFactory_update_missing_descriptor_sets);
    AI_CASE_RETURN(PipelineFactory_bottom_multiloop_while_loop);
    AI_CASE_RETURN(PipelineFactory_bottom_multiloop_for_loop);
    AI_CASE_RETURN(PipelineFactory_done);
  }
  AI_NEVER_REACHED
}

char const* PipelineFactory::task_name_impl() const
{
  return "PipelineFactory";
}

void PipelineFactory::characteristic_range_initialized()
{
  DoutEntering(dc::vulkan, "PipelineFactory::characteristic_range_initialized() [" << this << "]");
  if (m_number_of_running_characteristic_tasks.fetch_sub(1, std::memory_order::acq_rel) == 1)
    signal(characteristics_initialized);
}

void PipelineFactory::characteristic_range_filled(CharacteristicRangeIndex characteristic_range_index)
{
  DoutEntering(dc::vulkan, "PipelineFactory::characteristic_range_filled(" << characteristic_range_index << ") [" << this << "]");
  // Calculate the m_pipeline_index.
  size_t cr_index = characteristic_range_index.get_value();
  m_characteristics[characteristic_range_index]->update(pipeline_index_t::wat{m_pipeline_index}, cr_index, m_range_counters[cr_index], m_range_shift[characteristic_range_index]);
  if (m_number_of_running_characteristic_tasks.fetch_sub(1, std::memory_order::acq_rel) == 1)
    signal(characteristics_filled);
}

void PipelineFactory::characteristic_range_preprocessed()
{
  DoutEntering(dc::vulkan, "PipelineFactory::characteristic_range_preprocessed() [" << this << "]");
  if (m_number_of_running_characteristic_tasks.fetch_sub(1, std::memory_order::acq_rel) == 1)
    signal(characteristics_preprocessed);
}

void PipelineFactory::characteristic_range_compiled()
{
  DoutEntering(dc::vulkan, "PipelineFactory::characteristic_range_compiled() [" << this << "]");
  if (m_number_of_running_characteristic_tasks.fetch_sub(1, std::memory_order::acq_rel) == 1)
    signal(characteristics_compiled);
}

// Called from prepare_shader_resource_declarations.
void PipelineFactory::fill_set_index_hints(required_shader_resource_plus_characteristic_list_t::rat const& required_shader_resource_plus_characteristic_list_r, utils::Vector<vulkan::descriptor::SetIndexHint, ShaderResourcePlusCharacteristicIndex>& set_index_hints_out)
{
  DoutEntering(dc::vulkan, "PipelineFactory::fill_set_index_hints(" << set_index_hints_out << ") [" << this << "]");

  using namespace vulkan;
  using namespace pipeline::partitions;

  vulkan::LogicalDevice const* logical_device = m_owning_window->logical_device();
  int const number_of_elements = required_shader_resource_plus_characteristic_list_r->size();

  PartitionTask partition_task(number_of_elements, logical_device);

  // Run over all required shader resources.
  for (pipeline::ShaderResourcePlusCharacteristicIndex shader_resource_plus_characteristic_index1 = required_shader_resource_plus_characteristic_list_r->ibegin();
      shader_resource_plus_characteristic_index1 != required_shader_resource_plus_characteristic_list_r->iend(); ++shader_resource_plus_characteristic_index1)
  {
    auto const& shader_resource_plus_characteristic1 = (*required_shader_resource_plus_characteristic_list_r)[shader_resource_plus_characteristic_index1];
    for (pipeline::ShaderResourcePlusCharacteristicIndex shader_resource_plus_characteristic_index2 = required_shader_resource_plus_characteristic_list_r->ibegin();
        shader_resource_plus_characteristic_index2 != required_shader_resource_plus_characteristic_list_r->iend(); ++shader_resource_plus_characteristic_index2)
    {
      if (shader_resource_plus_characteristic_index1 == shader_resource_plus_characteristic_index2)
        continue;

      auto const& shader_resource_plus_characteristic2 = (*required_shader_resource_plus_characteristic_list_r)[shader_resource_plus_characteristic_index2];
      double preferred_preference = 0.0;
      for (std::vector<descriptor::SetKeyPreference>::const_iterator preferred = shader_resource_plus_characteristic1.m_preferred_descriptor_sets.begin();
          preferred != shader_resource_plus_characteristic1.m_preferred_descriptor_sets.end(); ++preferred)
      {
        if (preferred->descriptor_set_key() == shader_resource_plus_characteristic2.m_shader_resource_plus_characteristic.shader_resource()->descriptor_set_key())
        {
          preferred_preference = preferred->preference();
          break;
        }
      }

      double undesirable_preference = 0.0;
      for (std::vector<descriptor::SetKeyPreference>::const_iterator undesirable = shader_resource_plus_characteristic1.m_undesirable_descriptor_sets.begin();
          undesirable != shader_resource_plus_characteristic1.m_undesirable_descriptor_sets.end(); ++undesirable)
      {
        if (undesirable->descriptor_set_key() == shader_resource_plus_characteristic2.m_shader_resource_plus_characteristic.shader_resource()->descriptor_set_key())
        {
          undesirable_preference = undesirable->preference();
          break;
        }
      }
      // There can only be one descriptor that is an unbounded array in any descriptor set.
      if (shader_resource_plus_characteristic1.m_shader_resource_plus_characteristic.has_unbounded_descriptor_array_size() &&
          shader_resource_plus_characteristic2.m_shader_resource_plus_characteristic.has_unbounded_descriptor_array_size())
      {
        undesirable_preference = 1.0;
        // You can't demand that two decriptors that are unbounded arrays are in the same descriptor set.
        ASSERT(preferred_preference < 0.999);
      }

      // Looking for a formula such that,
      //   f(p, 0) = p
      //   f(0, u) = -u
      //   f(x, x) = 0
      //   f(1, x) = 1  for x < 1
      //   f(x, 1) = -1 for x < 1
      double preference = (preferred_preference - undesirable_preference) / (1.0 - preferred_preference * undesirable_preference);

      if (preference == 0.0)
        continue;

      Score score;

      if (utils::almost_equal(preference, 1.0, 1e-7))
        score = positive_inf;
      else if (utils::almost_equal(preference, -1.0, 1e-7))
        score = negative_inf;
      else
      {
        // 1 --> inf
        // 0 --> 0
        // -1 --> -inf
        score = preference / (1.0 - preference * preference);
      }

      // ElementIndex is equivalent to ShaderResourcePlusCharacteristicIndex.
      ASSERT(shader_resource_plus_characteristic_index2.get_value() < max_number_of_elements);
      ElementIndex e1{ElementIndexPOD{static_cast<int8_t>(shader_resource_plus_characteristic_index1.get_value())}};
      ElementIndex e2{ElementIndexPOD{static_cast<int8_t>(shader_resource_plus_characteristic_index2.get_value())}};
      ElementPair ep(e1, e2);
      int score_index = ep.score_index();
      Score existing_score = partition_task.score(score_index);
      if (!existing_score.is_zero())
      {
        if (!existing_score.is_infinite())
        {
          if (!score.is_infinite())
          {
            double existing_preference = (-1.0 / existing_score.value() +
                std::sqrt(1.0 / (existing_score.value() * existing_score.value()) + 4)) * 0.5;
            double new_preference = (existing_preference + preference) / (1.0 + existing_preference * preference);
            score = new_preference / (1.0 - new_preference * new_preference);
          }
        }
        else if (!score.is_infinite())
          score = existing_score;
        else
          // Can not demand two shader resources to be in the same descriptor set and not be in the same descriptor set.
          ASSERT(score.is_positive_inf() == existing_score.is_positive_inf());
      }
      Dout(dc::notice, "Assigned score for {" << shader_resource_plus_characteristic1.m_shader_resource_plus_characteristic.shader_resource()->debug_name() <<
          ", " << shader_resource_plus_characteristic2.m_shader_resource_plus_characteristic.shader_resource()->debug_name() << "} = " << score);
      partition_task.set_score(score_index, score);
    }
  }

  partition_task.initialize_set23_to_score();

  Score best_score(negative_inf);
  Partition best_partition;
  int best_rp = 0;
  int best_count = 0;
  //
  // 500 : { ... }
  // 503 : { ... }
  // 510 : { ... }
  // 550 : { ... }
  // 553 : { ... }
  // 553 : { ... }
  //
  //
  //
  //
  //
  //
  for (int rp = 0; rp < 1000; ++rp)
  {
    Partition partition = partition_task.random();
    Score score = partition.find_local_maximum(partition_task);
    int count = 0;
    for (PartitionIteratorExplode explode = partition.sbegin(partition_task); !explode.is_end(); ++explode)
    {
      Partition partition2 = explode.get_partition(partition_task);
      Score score2 = partition2.find_local_maximum(partition_task);
      if (score2 > score)
      {
        partition = partition2;
        score = score2;
      }
      if (++count == PartitionIteratorExplode::total_loop_count_limit)
        break;
    }
    if (score > best_score)
    {
      best_partition = partition;
      best_score = score;
      best_rp = rp;
      best_count = 1;
    }
    else if (score.unchanged(best_score))
      ++best_count;
    // Stop when the chance to have missed the best score until now would have been less than 1%, but no
    // sooner than after 20 attempts and only if the last half of all attempts didn't give an improvement.
    if (rp > std::max(20, 2 * best_rp) && std::exp(rp * std::log(static_cast<double>(rp - best_count) / rp)) < 0.01)
      break;
  }

  // Run over all required shader resources.
  set_index_hints_out.reserve(required_shader_resource_plus_characteristic_list_r->size());
  for (pipeline::ShaderResourcePlusCharacteristicIndex shader_resource_plus_characteristic_index = required_shader_resource_plus_characteristic_list_r->ibegin();
      shader_resource_plus_characteristic_index != required_shader_resource_plus_characteristic_list_r->iend(); ++shader_resource_plus_characteristic_index)
  {
    // partitions::ElementIndex is equivalent to ShaderResourcePlusCharacteristicIndex.
    ElementIndex element_index{ElementIndexPOD{static_cast<int8_t>(shader_resource_plus_characteristic_index.get_value())}};
    // partitions::SetIndex is equivalent to SetIndexHint.
    set_index_hints_out.push_back(descriptor::SetIndexHint{static_cast<size_t>(best_partition.set_of(element_index))});
  }
}

void PipelineFactory::prepare_shader_resource_declarations()
{
  DoutEntering(dc::vulkan, "PipelineFactory::prepare_shader_resource_declarations() [" << this << "]");

  utils::Vector<vulkan::descriptor::SetIndexHint, pipeline::ShaderResourcePlusCharacteristicIndex> set_index_hints;

  required_shader_resource_plus_characteristic_list_t::rat required_shader_resource_plus_characteristic_list_r(m_required_shader_resource_plus_characteristic_list);
  if (required_shader_resource_plus_characteristic_list_r->size() > 1)
    fill_set_index_hints(required_shader_resource_plus_characteristic_list_r, set_index_hints);
  else
    set_index_hints.emplace_back(0UL);

  for (pipeline::ShaderResourcePlusCharacteristicIndex shader_resource_plus_characteristic_index = required_shader_resource_plus_characteristic_list_r->ibegin();
      shader_resource_plus_characteristic_index != required_shader_resource_plus_characteristic_list_r->iend(); ++shader_resource_plus_characteristic_index)
  {
    shader_builder::ShaderResourceBase const* shader_resource = (*required_shader_resource_plus_characteristic_list_r)[shader_resource_plus_characteristic_index].m_shader_resource_plus_characteristic.shader_resource();
    m_set_key_to_set_index_hint[shader_resource->descriptor_set_key()] = set_index_hints[shader_resource_plus_characteristic_index];
    if (!shader_resource->is_prepared())
      shader_resource->prepare_shader_resource_declaration(set_index_hints[shader_resource_plus_characteristic_index], m_add_shader_stage);
  }
}

void PipelineFactory::multiplex_impl(state_type run_state)
{
#ifdef CWDEBUG
  // Turn dc::shaderresource on only if mSMDebug is on.
  bool const was_on = DEBUGCHANNELS::dc::shaderresource.is_on();
  bool const was_changed = mSMDebug != was_on;
  if (was_changed)
  {
    if (was_on)
      DEBUGCHANNELS::dc::shaderresource.off();
    else
      DEBUGCHANNELS::dc::shaderresource.on();
  }
  // Call this when the function is left.
  auto&& restore_shaderresource = at_scope_end([=](){
    if (was_changed)
    {
      if (was_on)
        DEBUGCHANNELS::dc::shaderresource.on();
      else
        DEBUGCHANNELS::dc::shaderresource.off();
    }
  });
#endif
  for (;;)
  {
    switch (run_state)
    {
      case PipelineFactory_start:
      {
        // Create our task::PipelineCache object.
        m_pipeline_cache_task = statefultask::create<PipelineCache>(this COMMA_CWDEBUG_ONLY(mSMDebug));
        m_pipeline_cache_task->run(vulkan::Application::instance().low_priority_queue(), this, pipeline_cache_set_up);
        // Wait until the pipeline cache is ready, then continue with PipelineFactory_initialize.
        set_state(PipelineFactory_initialize);
        wait(pipeline_cache_set_up);
        return;
      }
      case PipelineFactory_initialize:
        // Start a synchronous task that will be run when this task, that runs asynchronously, created a new pipeline and/or is finished.
        m_move_new_pipelines_synchronously = statefultask::create<synchronous::MoveNewPipelines>(m_owning_window, m_pipeline_factory_index COMMA_CWDEBUG_ONLY(mSMDebug));
        m_move_new_pipelines_synchronously->run();
        // Wait until the user is done adding CharacteristicRange objects and called generate().
        set_state(PipelineFactory_initialized);
        wait(fully_initialized);
        return;
      case PipelineFactory_initialized:
      {
        // Do not use an empty factory - it makes no sense.
        ASSERT(!m_characteristics.empty());
        size_t const number_of_characteristic_range_tasks = m_characteristics.size();
        m_range_shift.resize(number_of_characteristic_range_tasks);
        // The number of characteristic tasks that we need to wait for finishing initialization.
        m_number_of_running_characteristic_tasks.store(number_of_characteristic_range_tasks, std::memory_order::relaxed);
        // Call initialize on each characteristic.
        unsigned int range_shift = 0;
        for (auto i = m_characteristics.ibegin(); i != m_characteristics.iend(); ++i)
        {
          m_range_shift[i] = range_shift;
          range_shift += m_characteristics[i]->range_width();
          m_characteristics[i]->set_flat_create_info(&m_flat_create_info);
          m_characteristics[i]->run(vulkan::Application::instance().low_priority_queue());
        }
        set_state(PipelineFactory_characteristics_initialized);
        wait(characteristics_initialized);
        return;
      }
      case PipelineFactory_characteristics_initialized:
      {
        // Start as many for loops as there are characteristics.
        m_range_counters.initialize(m_characteristics.size(), (*m_characteristics.begin())->ibegin());
        // Enter the multi-loop.
        set_state(PipelineFactory_top_multiloop_for_loop);
        [[fallthrough]];
      }
      // The first time we enter from the top, m_range_counters.finished() should be false.
      // Therefore we can simply enter PipelineFactory_top_multiloop_for_loop, which replaces the
      // normal MultiLoop for-loop, regardless.
      //
      // This state mimics the usual MultiLoop magic:
      //   for(; !m_range_counters.finished(); m_range_counters.next_loop())
      case PipelineFactory_top_multiloop_for_loop:
        // We are now at the top of the for loop.
        //
        // This multiplex_impl function mimics the usual MultiLoop magic:
        //   while (m_range_counters() < m_characteristics[CharacteristicRangeIndex{*m_range_counters}]->iend())
        //   {
        if (!(m_range_counters() < m_characteristics[CharacteristicRangeIndex{*m_range_counters}]->iend()))
        {
          // We did not enter the while loop body.
          run_state = PipelineFactory_bottom_multiloop_for_loop;
          break;
        }
        set_state(PipelineFactory_top_multiloop_while_loop);
        [[fallthrough]];
      case PipelineFactory_top_multiloop_while_loop: // Having multiple for loops inside eachother.
        // We are now at the top of the while loop.
        //
        // Set m_start_of_next_loop to a random magic value to indicate that we are in the inner loop.
        m_start_of_next_loop = std::numeric_limits<int>::max();
        // Required MultiLoop magic.
        if (!m_range_counters.inner_loop())
        {
          // This is not the inner loop; set m_start_of_next_loop to the start value of the next loop.
          m_start_of_next_loop = m_characteristics[CharacteristicRangeIndex{*m_range_counters + 1}]->ibegin(); // Each loop, one per characteristic, runs from ibegin() till iend().
          // Since the whole reason that we're mimicking the for() while() MultiLoop magic with task states
          // is because we need to be able to leave and enter those loops in the middle of the inner loop,
          // we can not have local state (on the stack); might as well use a goto therefore ;).
          goto multiloop_magic_footer;
        }
        // MultiLoop inner loop body.

        pipeline_index_t::wat{m_pipeline_index}->set_to_zero();

        // The number of characteristic tasks that we need to wait for finishing do_fill.
        m_number_of_running_characteristic_tasks = 0;
        for (auto i = m_characteristics.ibegin(); i != m_characteristics.iend(); ++i)
          if ((m_characteristics[i]->needs_signals() & CharacteristicRange::do_fill))
            ++m_number_of_running_characteristic_tasks;
        ASSERT(m_number_of_running_characteristic_tasks == 1);
        // Run over each loop variable (and hence characteristic).
        for (auto i = m_characteristics.ibegin(); i != m_characteristics.iend(); ++i)
        {
          if ((m_characteristics[i]->needs_signals() & CharacteristicRange::do_fill))
          {
            // Do not accidently delete the parent task while still running a child task.
            // Call fill with its current range index.
            m_characteristics[i]->set_fill_index(m_range_counters[i.get_value()]);
            m_characteristics[i]->signal(CharacteristicRange::do_fill);
          }
        }
        set_state(PipelineFactory_characteristics_filled);
        wait(characteristics_filled);
        return;
      case PipelineFactory_characteristics_filled:
        prepare_shader_resource_declarations();
        // Now that we have added all shader variables, preprocess the shader code.
        // The number of characteristic tasks that we need to wait for finishing do_preprocess.
        m_number_of_running_characteristic_tasks = 0;
        for (auto i = m_characteristics.ibegin(); i != m_characteristics.iend(); ++i)
          if ((m_characteristics[i]->needs_signals() & CharacteristicRange::do_preprocess))
            ++m_number_of_running_characteristic_tasks;
        // Run over each loop variable (and hence characteristic).
        for (auto i = m_characteristics.ibegin(); i != m_characteristics.iend(); ++i)
        {
          if ((m_characteristics[i]->needs_signals() & CharacteristicRange::do_preprocess))
            m_characteristics[i]->signal(CharacteristicRange::do_preprocess);
        }
        set_state(PipelineFactory_characteristics_preprocessed);
        wait(characteristics_preprocessed);
        return;
      case PipelineFactory_characteristics_preprocessed:
        //-----------------------------------------------------------------
        // Begin pipeline layout creation

        {
          std::vector<vk::PushConstantRange> const sorted_push_constant_ranges = m_flat_create_info.get_sorted_push_constant_ranges();

          // Clear the m_set_index_hint_map of a previous fill.
          m_set_index_hint_map.clear();
          // Realize (create or get from cache) the pipeline layout and return a suitable SetIndexHintMap.
          m_vh_pipeline_layout = m_owning_window->logical_device()->realize_pipeline_layout(
              sorted_descriptor_set_layouts_t::wat(m_sorted_descriptor_set_layouts), m_set_index_hint_map, sorted_push_constant_ranges);
        }

        // Now that we have initialized m_set_index_hint_map, run the code that needs it.
        // The number of characteristic tasks that we need to wait for finishing do_compile.
        m_number_of_running_characteristic_tasks = 0;
        for (auto i = m_characteristics.ibegin(); i != m_characteristics.iend(); ++i)
          if ((m_characteristics[i]->needs_signals() & CharacteristicRange::do_compile))
            ++m_number_of_running_characteristic_tasks;
        // Run over each loop variable (and hence characteristic).
        for (auto i = m_characteristics.ibegin(); i != m_characteristics.iend(); ++i)
        {
          if ((m_characteristics[i]->needs_signals() & CharacteristicRange::do_compile))
          {
            m_characteristics[i]->set_set_index_hint_map(&m_set_index_hint_map);
            m_characteristics[i]->signal(CharacteristicRange::do_compile);
          }
        }
        set_state(PipelineFactory_characteristics_compiled);
        wait(characteristics_compiled);
        return;
      case PipelineFactory_characteristics_compiled:
        // All Characteristic(Range) tasks have finished their work for this pipeline at
        // this point and are waiting for the do_fill signal for the next pipeline.
        //
        // Sort the list of shader resources that are required for this pipeline.
        if (sort_required_shader_resources_list())
        {
          // No need to call PipelineFactory_create_shader_resources: there are no shader resources that need to be created.
          run_state = PipelineFactory_initialize_shader_resources_per_set_index;
          set_state(PipelineFactory_initialize_shader_resources_per_set_index);
          break;
        }
        set_state(PipelineFactory_create_shader_resources);
        [[fallthrough]];
      case PipelineFactory_create_shader_resources:
        if (!handle_shader_resource_creation_requests())
        {
          //PipelineFactory
          wait(obtained_create_lock);
          return;
        }
        set_state(PipelineFactory_initialize_shader_resources_per_set_index);
        [[fallthrough]];
      case PipelineFactory_initialize_shader_resources_per_set_index:
        m_have_lock = false;
        initialize_shader_resources_per_set_index();
        set_state(PipelineFactory_update_missing_descriptor_sets);
        [[fallthrough]];
      case PipelineFactory_update_missing_descriptor_sets:
        if (!update_missing_descriptor_sets())
        {
          m_have_lock = true;   // Next time we're woken up (by signal obtained_set_layout_binding_lock) we'll have the lock.
          wait(obtained_set_layout_binding_lock);
          return;
        }

        // End pipeline layout creation
        //-----------------------------------------------------------------

        // At this point all Characteristics must have finished.
        // Create the (next) pipeline...
        {
          // Merge the results of all characteristics into local vectors.
          std::vector<vk::VertexInputBindingDescription>     const vertex_input_binding_descriptions      = m_flat_create_info.get_vertex_input_binding_descriptions();
          std::vector<vk::VertexInputAttributeDescription>   const vertex_input_attribute_descriptions    = m_flat_create_info.get_vertex_input_attribute_descriptions();
          std::vector<vk::PipelineShaderStageCreateInfo>     const pipeline_shader_stage_create_infos     = m_flat_create_info.get_pipeline_shader_stage_create_infos();
          std::vector<vk::PipelineColorBlendAttachmentState> const pipeline_color_blend_attachment_states = m_flat_create_info.get_pipeline_color_blend_attachment_states();
          std::vector<vk::DynamicState>                      const dynamic_state                          = m_flat_create_info.get_dynamic_states();

          vk::PipelineVertexInputStateCreateInfo pipeline_vertex_input_state_create_info{
            .vertexBindingDescriptionCount = static_cast<uint32_t>(vertex_input_binding_descriptions.size()),
            .pVertexBindingDescriptions = vertex_input_binding_descriptions.data(),
            .vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_input_attribute_descriptions.size()),
            .pVertexAttributeDescriptions = vertex_input_attribute_descriptions.data()
          };

          vk::PipelineDynamicStateCreateInfo pipeline_dynamic_state_create_info{
            .dynamicStateCount = static_cast<uint32_t>(dynamic_state.size()),
            .pDynamicStates = dynamic_state.data()
          };

          vk::GraphicsPipelineCreateInfo pipeline_create_info{
            .stageCount = static_cast<uint32_t>(pipeline_shader_stage_create_infos.size()),
            .pStages = pipeline_shader_stage_create_infos.data(),
            .pVertexInputState = &pipeline_vertex_input_state_create_info,
            .pInputAssemblyState = &m_flat_create_info.m_pipeline_input_assembly_state_create_info,
            .pTessellationState = nullptr,
            .pViewportState = &m_flat_create_info.m_viewport_state_create_info,
            .pRasterizationState = &m_flat_create_info.m_rasterization_state_create_info,
            .pMultisampleState = &m_flat_create_info.m_multisample_state_create_info,
            .pDepthStencilState = &m_flat_create_info.m_depth_stencil_state_create_info,
            .pColorBlendState = &m_flat_create_info.m_color_blend_state_create_info,
            .pDynamicState = &pipeline_dynamic_state_create_info,
            .layout = m_vh_pipeline_layout,
            .renderPass = m_vh_render_pass,
            .subpass = 0,
            .basePipelineHandle = vk::Pipeline{},
            .basePipelineIndex = -1
          };

#ifdef CWDEBUG
          Dout(dc::vulkan|continued_cf, "PipelineFactory [" << this << "] creating graphics pipeline with range values: ");
          char const* prefix = "";
          for (int i = 0; i < m_characteristics.size(); ++i)
          {
            Dout(dc::continued, prefix << m_range_counters[i]);
            prefix = ", ";
          }
          Dout(dc::finish, " --> pipeline::Index " << *pipeline_index_t::rat{m_pipeline_index});
#endif

          // Create and then store the graphics pipeline.
          vk::UniquePipeline pipeline = m_owning_window->logical_device()->create_graphics_pipeline(m_pipeline_cache_task->vh_pipeline_cache(), pipeline_create_info
              COMMA_CWDEBUG_ONLY(m_owning_window->debug_name_prefix("pipeline")));

          // Inform the SynchronousWindow.
          m_move_new_pipelines_synchronously->have_new_datum({vulkan::Pipeline{m_vh_pipeline_layout, {m_pipeline_factory_index, *pipeline_index_t::rat{m_pipeline_index}}, m_descriptor_set_per_set_index, m_owning_window->max_number_of_frame_resources()
              COMMA_CWDEBUG_ONLY(m_owning_window->logical_device())}, std::move(pipeline)});
        }

        //
        // End of MultiLoop inner loop.
multiloop_magic_footer:
        // Required MultiLoop magic.
        m_range_counters.start_next_loop_at(m_start_of_next_loop);
        // If this is true then we're at the end of the inner loop and want to yield.
        if (m_start_of_next_loop == std::numeric_limits<int>::max() && !m_range_counters.finished())
        {
          yield();
          set_state(PipelineFactory_bottom_multiloop_while_loop);
          return;     // Yield and then continue here --------.
        }                                               //    |
      case PipelineFactory_bottom_multiloop_while_loop: // <--'
        if (m_range_counters() < m_characteristics[CharacteristicRangeIndex{*m_range_counters}]->iend())
        {
          run_state = PipelineFactory_top_multiloop_while_loop;
          set_state(PipelineFactory_top_multiloop_while_loop);
          break;
        }
        [[fallthrough]];
      case PipelineFactory_bottom_multiloop_for_loop:
        m_range_counters.next_loop();
        if (!m_range_counters.finished())
        {
          set_state(PipelineFactory_top_multiloop_for_loop);
          run_state = PipelineFactory_top_multiloop_for_loop;
          break;
        }
        set_state(PipelineFactory_done);
        [[fallthrough]];
      case PipelineFactory_done:
        finish();
        return;
    }
  }
}

void PipelineFactory::finish_impl()
{
  // The characteristic range tasks never stopped running. We must kill them.
  for (auto i = m_characteristics.ibegin(); i != m_characteristics.iend(); ++i)
    m_characteristics[i]->terminate();
  m_move_new_pipelines_synchronously->set_producer_finished();
}

struct CompareShaderResourcePlusCharacteristic
{
  bool operator()(ShaderResourcePlusCharacteristic const& spc1, ShaderResourcePlusCharacteristic const& spc2)
  {
    return spc1.shader_resource() < spc2.shader_resource();
  }
};

// Called from PipelineFactory_characteristics_compiled.
bool PipelineFactory::sort_required_shader_resources_list()
{
  DoutEntering(dc::vulkan, "PipelineFactory::sort_required_shader_resources_list() [" << this << "]");

  required_shader_resource_plus_characteristic_list_t::wat required_shader_resource_plus_characteristic_list_w(m_required_shader_resource_plus_characteristic_list);

  // Seems impossible that one of these is empty if the other isn't.
  ASSERT(required_shader_resource_plus_characteristic_list_w->empty() == sorted_descriptor_set_layouts_t::rat(m_sorted_descriptor_set_layouts)->empty());

  if (required_shader_resource_plus_characteristic_list_w->empty())
  {
    // Initialize m_set_index_end because we won't call handle_shader_resource_creation_requests.
    m_set_index_end.set_to_zero();
    return true;        // Nothing to do.
  }

  struct Compare
  {
    bool operator()(ShaderResourcePlusCharacteristicPlusPreferredAndUndesirableSetKeyPreferences const& spc1,
                    ShaderResourcePlusCharacteristicPlusPreferredAndUndesirableSetKeyPreferences const& spc2)
    {
      return spc1.m_shader_resource_plus_characteristic.shader_resource() < spc2.m_shader_resource_plus_characteristic.shader_resource();
    }
  };

  // The used resources need to have some global order for the "horse race" algorithm to work.
  // Sorting them by ShaderResourceBase* will do.
  std::sort(required_shader_resource_plus_characteristic_list_w->begin(), required_shader_resource_plus_characteristic_list_w->end(), Compare{});

  // Continue with calling handle_shader_resource_creation_requests.
  return false;
}

// Called by ShaderResourceDeclarationContext::generate1 which is
// called from preprocess1.
void PipelineFactory::push_back_descriptor_set_layout_binding(vulkan::descriptor::SetIndexHint set_index_hint, vk::DescriptorSetLayoutBinding const& descriptor_set_layout_binding, vk::DescriptorBindingFlags binding_flags, int32_t descriptor_array_size,
    utils::Badge<vulkan::shader_builder::ShaderResourceDeclarationContext>)
{
  DoutEntering(dc::vulkan, "PipelineFactory::push_back_descriptor_set_layout_binding(" << set_index_hint << ", " << descriptor_set_layout_binding << ", " << binding_flags << ", " << descriptor_array_size << ") [" << this << "]");
  Dout(dc::vulkan, "Adding " << descriptor_set_layout_binding << " to m_sorted_descriptor_set_layouts[" << set_index_hint << "].m_sorted_bindings_and_flags:");
  // Find the SetLayout corresponding to the set_index_hint, if any.
  sorted_descriptor_set_layouts_t::wat sorted_descriptor_set_layouts_w(m_sorted_descriptor_set_layouts);
  auto set_layout = std::find_if(sorted_descriptor_set_layouts_w->begin(), sorted_descriptor_set_layouts_w->end(), descriptor::CompareHint{set_index_hint});
  if (set_layout == sorted_descriptor_set_layouts_w->end())
  {
    sorted_descriptor_set_layouts_w->emplace_back(set_index_hint);
    set_layout = sorted_descriptor_set_layouts_w->end() - 1;
  }
  set_layout->insert_descriptor_set_layout_binding(descriptor_set_layout_binding, binding_flags, descriptor_array_size);
  // set_layout is an element of m_sorted_descriptor_set_layouts and it was just changed.
  // We need to re-sort m_sorted_descriptor_set_layouts to keep it sorted.
  std::sort(sorted_descriptor_set_layouts_w->begin(), sorted_descriptor_set_layouts_w->end(), descriptor::SetLayoutCompare{});
}

// Called from PipelineFactory_create_shader_resources.
//
// Returns true if this PipelineFactory task succeeded in creating all shader resources needed by the current pipeline (being created),
// and false if one or more other tasks are creating one or more of those shader resources at this moment.
// Upon successful return m_descriptor_set_per_set_index was initialized with handles to the descriptor sets needed by the current pipeline.
// Additionally, those descriptor sets are updated to bind to the respective shader resources. The descriptor sets are created
// as needed: if there already exists a descriptor set that is bound to the same shader resources that we need, we just reuse
// that descriptor set.
bool PipelineFactory::handle_shader_resource_creation_requests()
{
  DoutEntering(dc::shaderresource|dc::vulkan, "PipelineFactory::handle_shader_resource_creation_requests() [" << this << "]");

  using namespace vulkan::descriptor;
  using namespace vulkan::shader_builder::shader_resource;

  // Just (re)initialize m_set_index_end every time we (re)enter this function.
  m_set_index_end.set_to_zero();
  m_set_index.set_to_zero();

  // Try to be the first to get 'ownership' on each shader resource that has to be created.
  // This is achieved in the following way:
  // m_required_shader_resource_plus_characteristic_list is sorted by (unique) pointer value of the shader resources
  // so that every pipeline factory will try to process them in the same order.
  // If, for example, pipeline factory 0 tries to create A, B, C, D; and pipeline factory 1
  // tries to create B, D, E then one that locks B first wins. And if at the same time pipeline
  // factory 2 tries to create C, E, then the winner of B competes with 2 in a race for
  // C or E respectively. I dubbed this the "race horse" algorithm because each shader resource
  // plays the role of a finish line:
  //
  // 0 ---A-B-C-D---->  needs to lock B before 1 and then C before 2.
  //        | | |
  // 1 -----B-|-D-E-->  needs to lock B before 0 and then E before 2.
  //          |   |
  // 2 -------C---E-->  needs to lock C before 0 and E before 1.
  //
  required_shader_resource_plus_characteristic_list_t::rat required_shader_resource_plus_characteristic_list_r(m_required_shader_resource_plus_characteristic_list);
  for (auto&& shader_resource_plus_characteristic : *required_shader_resource_plus_characteristic_list_r)
  {
    ShaderResourceBase const* shader_resource = shader_resource_plus_characteristic.m_shader_resource_plus_characteristic.shader_resource();
    SetKey const set_key = shader_resource->descriptor_set_key();
    SetIndexHint const set_index_hint = get_set_index_hint(set_key);
    SetIndex const set_index = m_set_index_hint_map.convert(set_index_hint);

    if (set_index >= m_set_index_end)
      m_set_index_end = set_index + 1;

    if (shader_resource->is_created())  // Skip shader resources that are already created.
      continue;

    // Try to get the lock on all other shader resources.
    if (!shader_resource->acquire_create_lock(this, task::PipelineFactory::obtained_create_lock))
    {
      // If some other task was first, then we back off by returning false.
      return false;
    }
    // Now that we have the exclusive lock - it is safe to const_cast the const-ness away.
    // This is only safe because no other thread will be reading the (non-atomic) members
    // of the shader resources that we are creating while we are creating them.
    m_acquired_required_shader_resources_list.push_back(const_cast<ShaderResourceBase*>(shader_resource));
  }

#ifdef CWDEBUG
  int index = 0;
#endif
  for (ShaderResourceBase* shader_resource : m_acquired_required_shader_resources_list)
  {
    // Create the shader resource (if not already created (e.g. UniformBuffer)).
    shader_resource->instantiate(m_owning_window COMMA_CWDEBUG_ONLY(m_owning_window->debug_name_prefix(shader_resource->debug_name())));

    // Let other pipeline factories know that this shader resource was already created.
    // It might be used immediately, even during this call.
    shader_resource->set_created();
    Debug(++index);
  }

  // Release all the task-mutexes, in reverse order.
  for (auto shader_resource = m_acquired_required_shader_resources_list.rbegin(); shader_resource != m_acquired_required_shader_resources_list.rend(); ++shader_resource)
    (*shader_resource)->release_create_lock();

  return true;
}

// Called from PipelineFactory_initialize_shader_resources_per_set_index.
void PipelineFactory::initialize_shader_resources_per_set_index()
{
  DoutEntering(dc::vulkan, "PipelineFactory::initialize_shader_resources_per_set_index() [" << this << "]");
  Dout(dc::shaderresource, "m_set_index_end = " << m_set_index_end);

  using namespace vulkan::descriptor;
  using namespace vulkan::shader_builder::shader_resource;

  m_shader_resource_plus_characteristics_per_set_index.resize(m_set_index_end.get_value());
  Dout(dc::shaderresource, "Running over all m_required_shader_resource_plus_characteristic_list shader_resource's:");
  required_shader_resource_plus_characteristic_list_t::rat required_shader_resource_plus_characteristic_list_r(m_required_shader_resource_plus_characteristic_list);
  for (auto&& shader_resource_plus_characteristic : *required_shader_resource_plus_characteristic_list_r)
  {
    ShaderResourceBase const* shader_resource = shader_resource_plus_characteristic.m_shader_resource_plus_characteristic.shader_resource();
    SetKey const set_key = shader_resource->descriptor_set_key();
    SetIndexHint const set_index_hint = get_set_index_hint(set_key);
    SetIndex const set_index = m_set_index_hint_map.convert(set_index_hint);
    Dout(dc::shaderresource, "  shader_resource " << shader_resource << " (" << NAMESPACE_DEBUG::print_string(shader_resource->debug_name()) <<
        ") has set_index_hint = " << set_index_hint << ", set_index = " << set_index);
    m_shader_resource_plus_characteristics_per_set_index[set_index].push_back(shader_resource_plus_characteristic.m_shader_resource_plus_characteristic);
  }
  // Sort the vectors inside m_shader_resource_plus_characteristics_per_set_index so that pipeline factories that attempt
  // to create the same descriptor set will process the associated shader resources in the same order.
  for (auto&& shader_resource_plus_characteristic : *required_shader_resource_plus_characteristic_list_r)
  {
    ShaderResourceBase const* shader_resource = shader_resource_plus_characteristic.m_shader_resource_plus_characteristic.shader_resource();
    SetKey const set_key = shader_resource->descriptor_set_key();
    SetIndexHint const set_index_hint = get_set_index_hint(set_key);
    SetIndex const set_index = m_set_index_hint_map.convert(set_index_hint);
    std::sort(
        m_shader_resource_plus_characteristics_per_set_index[set_index].begin(),
        m_shader_resource_plus_characteristics_per_set_index[set_index].end(),
        CompareShaderResourcePlusCharacteristic{});
  }

  // We're going to fill this vector now.
  ASSERT(m_descriptor_set_per_set_index.empty());
  m_descriptor_set_per_set_index.resize(m_set_index_end.get_value());
}

// Called from PipelineFactory_update_missing_descriptor_sets.
bool PipelineFactory::update_missing_descriptor_sets()
{
  DoutEntering(dc::shaderresource|dc::vulkan, "PipelineFactory::update_missing_descriptor_sets() [" << this << "]");

  using namespace vulkan::descriptor;
  using namespace vulkan::shader_builder::shader_resource;

  std::vector<vk::DescriptorSetLayout> missing_descriptor_set_layouts;          // The descriptor set layout handles of descriptor sets that we need to create.
  std::vector<uint32_t> missing_descriptor_set_unbounded_descriptor_array_sizes; // Whether or not that descriptor set layout contains a descriptor that is an unbounded array.
  std::vector<std::pair<SetIndex, bool>> set_index_has_frame_resource_pairs;    // The corresponding SetIndex-es, and a flag indicating if this is a frame resource, of those descriptor sets.

  {
    sorted_descriptor_set_layouts_t::rat sorted_descriptor_set_layouts_r(m_sorted_descriptor_set_layouts);

    // This function is called after realize_descriptor_set_layouts which means that the `binding` values
    // in the elements of SetLayout::m_sorted_bindings_and_flags, of each SetLayout in m_sorted_descriptor_set_layouts,
    // is already finalized.
    Dout(dc::vulkan, "m_sorted_descriptor_set_layouts = " << *sorted_descriptor_set_layouts_r);
    utils::Vector<vk::DescriptorSetLayout, SetIndex> vhv_descriptor_set_layouts(sorted_descriptor_set_layouts_r->size());
    utils::Vector<uint32_t, SetIndex> unbounded_descriptor_array_sizes(sorted_descriptor_set_layouts_r->size());
    for (auto& descriptor_set_layout : *sorted_descriptor_set_layouts_r)
    {
      descriptor::SetIndex set_index = m_set_index_hint_map.convert(descriptor_set_layout.set_index_hint());
      // This code assumes that m_sorted_descriptor_set_layouts contains contiguous range [0, 1, 2, ..., size-1] of
      // set index hint values - one for each.
      ASSERT(set_index < vhv_descriptor_set_layouts.iend());
      vhv_descriptor_set_layouts[set_index] = descriptor_set_layout.handle();
      unbounded_descriptor_array_sizes[set_index] = descriptor_set_layout.sorted_bindings_and_flags().unbounded_descriptor_array_size();
    }

    // Isn't this ALWAYS the case? Note that it might take a complex application that is skipping
    // shader resources that are in a given descriptor set but aren't used in this pipeline; hence
    // leave this assert here for a very long time.
    ASSERT(m_set_index_end == vhv_descriptor_set_layouts.iend());

    // Descriptor sets: { U1, T1 }, { U2, T2 }, { T1, T2 }, { T2, T1 }, { U1, U2 }, { U2, U1 }, { U1, T2 }, { U2, T1 }
    // Pipeline layout:                   SetLayout
    //   ({ U, T }, { U, T }),            (#0{ U, T }, #1{ U, T })
    //   ({ U, U }, { T, T }),            (#0{ U, U }, #1{ T, T })
    //   ({ T, T }, { U, U })             (#0{ T, T }, #1{ U, U })
    // Pipelines:
    //   ({ U1, T1 }, { U2, T2 }),
    //   ({ U1, T2 }, { U2, T1 }),
    //   ({ U2, T1 }, { U1, T2 }),
    //   ({ U2, T2 }, { U1, T1 }),
    //   ({ U1, U2 }, { T1, T2 }),
    //   ({ U1, U2 }, { T2, T1 }),
    //   ({ U2, U1 }, { T2, T1 }),  <-- 1st      #0 -> #0, #1 -> #1       U2 -> <#0{ U, U }, 0>,
    //                                                                    U1 -> <#0{ U, U }, 1>,
    //                                                                    T2 -> <#1{ T, T }, 0>,
    //                                                                    T1 -> <#1{ T, T }, 1>
    //   ({ U2, U1 }, { T1, T2 }),
    //   ({ T1, T2 }, { U1, U2 }),
    //   ({ T1, T2 }, { U2, U1 }),  <-- 2nd      #0 -> #1, #1 -> #0       T1 -> <#0{ T, T }, 0>,
    //                                                                    T2 -> <#0{ T, T }, 1>,
    //                                                                    U2 -> <#1{ U, U }, 0>,
    //                                                                    U1 -> <#1{ U, U }, 1>
    //   ({ T2, T1 }, { U1, U2 }),
    //   ({ T2, T1 }, { U2, U1 })
    //
    //    U2 -> <#0{ U, U }, 0>,
    //    U1 -> <#0{ U, U }, 1>,
    // T  T2 -> <#1{ T, T }, 0>, <#0{ T, T }, 1>, ...
    // T  T1 -> <#1{ T, T }, 1>, <#0{ T, T }, 0>, ...
    //
    Dout(dc::shaderresource, "Locking and searching for existing descriptor sets: loop over set_index [" << m_set_index << " - " << m_set_index_end << ">");
    for (SetIndex set_index = m_set_index; set_index < m_set_index_end; ++set_index)    // #0, #1, #2, #3
    {
      // We want to know if there is a vk::DescriptorSetLayout that appears in every shader resource corresponding to this set_index.
      std::map<descriptor::FrameResourceCapableDescriptorSetAsKey, int> number_of_shader_resources_bound_to;
      int number_of_shader_resources_in_this_set_index = 0;
      descriptor::SetLayoutBinding locked_set_layout_binding{VK_NULL_HANDLE, 0};
      bool have_match = false;
      descriptor::FrameResourceCapableDescriptorSet existing_descriptor_set;
      // Shader resources: T1, T2, T3, T4, U1, U2, U3, U4 (each with a unique key)
      // PL0:              PL1:                    PL1 after conversion:
      // #0 : { U2, U1 }   #0 : { U4, U3 } -> #0   #0 : { U4, U3 }
      // #1 : { T2, T1 }   #1 : { U2, U1 } -> #2   #1 : { T4, T3 }
      // #2 : { U3, U4 }   #2 : { T1, T2 } -> #3   #2 : { U2, U1 }
      // #3 : { T4, T3 }   #3 : { T4, T3 } -> #1   #3 : { T1, T2 }                  // set index: #0        #1        #2        #3
      Dout(dc::shaderresource, "Loop over all shader resources with index " << set_index);
      bool first_shader_resource = true;
      bool has_frame_resource = false;
      auto shader_resource_plus_characteristic_iter = m_shader_resource_plus_characteristics_per_set_index[set_index].begin();
      for (; shader_resource_plus_characteristic_iter != m_shader_resource_plus_characteristics_per_set_index[set_index].end(); ++shader_resource_plus_characteristic_iter)
      {
        ShaderResourceBase const* shader_resource = shader_resource_plus_characteristic_iter->shader_resource();
        Dout(dc::shaderresource, "shader_resource = " << shader_resource << " (" << NAMESPACE_DEBUG::print_string(shader_resource->debug_name()) << ")");
        SetKey const set_key = shader_resource->descriptor_set_key();
        SetIndexHint const set_index_hint = get_set_index_hint(set_key);
        uint32_t binding = get_declaration(set_key)->binding();
        Dout(dc::shaderresource, "binding = " << binding);

        if (shader_resource->is_frame_resource())
          has_frame_resource = true;

        // Find the corresponding SetLayout.
        auto sorted_descriptor_set_layout = std::find_if(sorted_descriptor_set_layouts_r->begin(), sorted_descriptor_set_layouts_r->end(), CompareHint{set_index_hint});
        ASSERT(sorted_descriptor_set_layout != sorted_descriptor_set_layouts_r->end());
        // cp0 --> { U, U }, cp1 --> { T, T }
        vk::DescriptorSetLayout descriptor_set_layout = sorted_descriptor_set_layout->handle();
        Dout(dc::shaderresource, "shader_resource corresponds with descriptor_set_layout " << descriptor_set_layout);
        descriptor::SetLayoutBinding const set_layout_binding(descriptor_set_layout, binding);
        Dout(dc::shaderresource, "set_layout_binding (key) = " << set_layout_binding);
        // Assume a given set index requires four shader resources Rx.
        // Since m_shader_resource_plus_characteristics_per_set_index[set_index] is sorted by descriptor set layout handle, all pipeline
        // factories will process the shader resources in the same order; lets say R1, R2, R3 and finally R4.
        //
        // A set then might look like:
        //             { R3, R1, R2, R4 }
        //       binding: 0   1   2   3
        //                        |
        //                        |
        //                        v
        //                   {T,T,U,U}/2 --> SetMutexAndSetHandles = L{ handle1, handle2 }
        //  SetLayoutBinding:   layout/binding
        //
        // Note that SetLayoutBinding in reality stores a vk::DescriptorSetLayout handle, and not the layout itself, but
        // since we use a cache (LogicalDevice::m_descriptor_set_layouts) for descriptor set layouts, each unique layout
        // will have a unique handle.
        //
        // The function shader_resource->lock_set_layout_binding attempts to add a {SetLayoutBinding, SetMutexAndSetHandles}
        // pair to m_set_layout_bindings_to_handles using the descriptor set layout handle and binding that correspond
        // to the required descriptor set.
        //
        // If the key already exists, and the corresponding SetMutexAndSetHandles is locked, it returns false.
        // If it didn't exist, it was created. If the corresponding SetMutexAndSetHandles wasn't locked, it was
        // locked and returned true; this signifies that creation of this descriptor set might be in
        // progress.
        //
        // Hence the state of the first shader resource, R1, before calling lock_set_layout_binding is either:
        // 1) the key {descriptor_set_layout, binding} does not exist.
        // 2) the key exists with a SetMutexAndSetHandles that isn't locked.
        // 3) the key exists with a SetMutexAndSetHandles that is locked.
        //
        // In case 1) the key is created with an empty SetMutexAndSetHandles, which is locked.
        // In case 2) the corresponding SetMutexAndSetHandles is also locked.
        //
        // The current pipeline factory then should do the following:
        // 1,2) Continue with next shader resource until it we find a case 3) or finished checking all shader resources.
        // 3) Back off - until the lock is released. At which point we should continue as in 2). When the other task
        //    released the lock and this task acquired it, this task will re-enter update_missing_descriptor_sets from
        //    the top with m_have_lock set to true.

        // If this is not the first shader resource (R1) then it is impossible that other tasks are trying to create
        // the same descriptor set: they process the shader_resources involved in the same order and will stop when
        // trying the first shader resource (R1) or had stopped us if they were first. Therefore no lock needs to
        // obtained for R2, R3 and R4.
        bool in_progress =
          first_shader_resource &&        // Only lock the first shader_resource for every set_index.
          !m_have_lock &&                 // Already have the lock; no need to call lock_set_layout_binding.
          !shader_resource->lock_set_layout_binding(set_layout_binding, this, task::PipelineFactory::obtained_set_layout_binding_lock);
        if (first_shader_resource)
        {
#ifdef CWDEBUG
          if (m_have_lock)
            shader_resource->set_have_lock(set_layout_binding, this);
#endif
          first_shader_resource = false;
          Dout(dc::shaderresource, (in_progress ? "In progress!" : "Not in progress."));
          if (in_progress)
          {
            // If the first shader resource (R1) was already locked, and this is not the first
            // set_index that we processed this call (which is m_set_index) then continue with
            // handling the set indexes that we already processed: [m_set_index, set_index>.
            if (set_index > m_set_index)
            {
              // allocate_update_add_handles_and_unlocking takes a write lock.
              sorted_descriptor_set_layouts_r.unlock();
              allocate_update_add_handles_and_unlocking(
                  missing_descriptor_set_layouts, missing_descriptor_set_unbounded_descriptor_array_sizes,
                  set_index_has_frame_resource_pairs, m_set_index, set_index);
            }
            // Next time continue with the current set_index.
            m_set_index = set_index;
            return false; // Wait until the other pipeline factory unlocked shader_resource->m_set_layout_bindings_to_handles[set_layout_binding].
          }

          // Remember which set_layout_binding we locked for this set_index.
          locked_set_layout_binding = set_layout_binding;
        }

        ++number_of_shader_resources_in_this_set_index;
        bool still_have_match = false;
        {
          auto set_layout_bindings_to_handles_crat = shader_resource->set_layout_bindings_to_handles();
          auto set_layout_binding_handles_pair_iter = set_layout_bindings_to_handles_crat->find(set_layout_binding);
          if (set_layout_binding_handles_pair_iter == set_layout_bindings_to_handles_crat->end())
          {
            Dout(dc::shaderresource, "Setting have_match to false because " << set_layout_binding << " doesn't exist.");
            have_match = false;
            // Note: we need to keep the lock on set_layout_binding in this case (if we have it).
            break;
          }
          else
            Dout(dc::shaderresource, "Found SetMutexAndSetHandles for " << set_layout_binding);

          // Run over all descriptor::FrameResourceCapableDescriptorSet that this shader resource is already bound to.
          auto const& descriptor_sets = set_layout_binding_handles_pair_iter->second.descriptor_sets();
          Dout(dc::shaderresource, "Running over " << descriptor_sets.size() << " descriptor sets.");
          for (descriptor::FrameResourceCapableDescriptorSet const& descriptor_set : descriptor_sets)
          {
            Dout(dc::shaderresource, "Incrementing count for descriptor set " << descriptor_set);
            if (++number_of_shader_resources_bound_to[descriptor_set.as_key()] == number_of_shader_resources_in_this_set_index)
            {
              Dout(dc::shaderresource, "descriptor_set = " << descriptor_set << " and binding = " << binding << " leads to still_have_match = true.");
              have_match = still_have_match = true;
              existing_descriptor_set = descriptor_set;
              Dout(dc::shaderresource, "Set existing_descriptor_set to " << existing_descriptor_set);
            }
            else
            {
              Dout(dc::shaderresource, "number_of_shader_resources_in_this_set_index = " << number_of_shader_resources_in_this_set_index <<
                  " but number_of_shader_resources_bound_to[descriptor_set] now is only " << number_of_shader_resources_bound_to[descriptor_set.as_key()]);
            }
          }
        } // Destruct set_layout_bindings_to_handles_crat.

        if (!still_have_match)
        {
          Dout(dc::shaderresource, "Setting have_match to false because still_have_match is false.");
          have_match = false;
          break;
        }
      } // Next shader resource for this set_index.

      if (have_match)
      {
        m_descriptor_set_per_set_index[set_index] = existing_descriptor_set;
        // It should not be possible to get here without locking a set_layout_binding on a shader_resource.
        ASSERT(!m_shader_resource_plus_characteristics_per_set_index[set_index].empty() && locked_set_layout_binding.descriptor_set_layout());
        m_shader_resource_plus_characteristics_per_set_index[set_index].begin()->shader_resource()->unlock_set_layout_binding(locked_set_layout_binding
            COMMA_CWDEBUG_ONLY(this));
      }
      else
      {
        uint32_t unbounded_descriptor_array_size = unbounded_descriptor_array_sizes[set_index];
        if (!missing_descriptor_set_unbounded_descriptor_array_sizes.empty() || unbounded_descriptor_array_size != 0)
        {
          if (missing_descriptor_set_unbounded_descriptor_array_sizes.empty() && !missing_descriptor_set_layouts.empty())
            missing_descriptor_set_unbounded_descriptor_array_sizes.resize(missing_descriptor_set_layouts.size());
          missing_descriptor_set_unbounded_descriptor_array_sizes.push_back(unbounded_descriptor_array_size);
        }
        missing_descriptor_set_layouts.push_back(vhv_descriptor_set_layouts[set_index]);
        Dout(dc::shaderresource, "Adding set_index " << set_index << " to set_index_has_frame_resource_pairs because have_match is false.");
        // See if there is any shader resource in this set index that is a frame resource.
        for (; !has_frame_resource && shader_resource_plus_characteristic_iter != m_shader_resource_plus_characteristics_per_set_index[set_index].end();
            ++shader_resource_plus_characteristic_iter)
          if (shader_resource_plus_characteristic_iter->shader_resource()->is_frame_resource())
            has_frame_resource = true;
        set_index_has_frame_resource_pairs.emplace_back(set_index, has_frame_resource);
      }
      // This is only true when we just (re)entered this function, not for the next set_index.
      m_have_lock = false;
    }
  } // Unlock m_sorted_descriptor_set_layouts.

  allocate_update_add_handles_and_unlocking(missing_descriptor_set_layouts, missing_descriptor_set_unbounded_descriptor_array_sizes,
      set_index_has_frame_resource_pairs, m_set_index, m_set_index_end);
  return true;
}

// Called from update_missing_descriptor_sets.
void PipelineFactory::allocate_update_add_handles_and_unlocking(
    std::vector<vk::DescriptorSetLayout> const& missing_descriptor_set_layouts,
    std::vector<uint32_t> const& missing_descriptor_set_unbounded_descriptor_array_sizes,
    std::vector<std::pair<descriptor::SetIndex, bool>> const& set_index_has_frame_resource_pairs,
    descriptor::SetIndex set_index_begin, descriptor::SetIndex set_index_end)
{
  DoutEntering(dc::shaderresource|dc::vulkan, "PipelineFactory::allocate_update_add_handles_and_unlocking(" <<
      missing_descriptor_set_layouts << ", " << missing_descriptor_set_unbounded_descriptor_array_sizes << ", " <<
      set_index_has_frame_resource_pairs << ", " << set_index_begin << ", " << set_index_end << ") [" << this << "]");

  using namespace vulkan::descriptor;
  using namespace vulkan::shader_builder::shader_resource;

  vulkan::LogicalDevice const* logical_device = m_owning_window->logical_device();
  std::vector<FrameResourceCapableDescriptorSet> missing_descriptor_sets;
  if (!missing_descriptor_set_layouts.empty())
    missing_descriptor_sets = logical_device->allocate_descriptor_sets(
        m_owning_window->max_number_of_frame_resources(),
        missing_descriptor_set_layouts, missing_descriptor_set_unbounded_descriptor_array_sizes,
        set_index_has_frame_resource_pairs, logical_device->get_descriptor_pool()
        COMMA_CWDEBUG_ONLY(Ambifix{"PipelineFactory::m_descriptor_set_per_set_index", as_postfix(this)}));
           // Note: the debug name is changed when copying this vector to the Pipeline it will be used with.
  for (int i = 0; i < missing_descriptor_set_layouts.size(); ++i)
    m_descriptor_set_per_set_index[set_index_has_frame_resource_pairs[i].first] = missing_descriptor_sets[i];

  Dout(dc::shaderresource, "Updating, adding handles and unlocking: loop over set_index [" << set_index_begin << " - " << set_index_end << ">.");
  for (SetIndex set_index = set_index_begin; set_index < set_index_end; ++set_index)
  {
    // Run over all shader resources in reverse, so that the first one which is the one that is locked, is processed last!
    Dout(dc::shaderresource, "Loop over all shader resources - in reverse - with index " << set_index);
    for (int i = m_shader_resource_plus_characteristics_per_set_index[set_index].size() - 1; i >= 0; --i)
    {
      ShaderResourcePlusCharacteristic const& shader_resource_plus_characteristic = m_shader_resource_plus_characteristics_per_set_index[set_index][i];
      ShaderResourceBase const* shader_resource = shader_resource_plus_characteristic.shader_resource();
      bool first_shader_resource = i == 0;
      SetKey const set_key = shader_resource->descriptor_set_key();
      SetIndexHint const set_index_hint = get_set_index_hint(set_key);
      ASSERT(set_index == m_set_index_hint_map.convert(set_index_hint));
      uint32_t binding = get_declaration(set_key)->binding();
      Dout(dc::shaderresource, "shader_resource = " << shader_resource << " (" << NAMESPACE_DEBUG::print_string(shader_resource->debug_name()) <<
          "); set_index = " << set_index << "; binding = " << binding);

      auto new_descriptor_set = std::find_if(set_index_has_frame_resource_pairs.begin(), set_index_has_frame_resource_pairs.end(),
          [=](std::pair<SetIndex, bool> const& p) { return p.first == set_index; });
      if (new_descriptor_set != set_index_has_frame_resource_pairs.end())
      {
        Dout(dc::shaderresource, "The set_index was found in set_index_has_frame_resource_pairs.");
        // Bind it to a descriptor set.
        // Note: only one PipelineFactory does the update_descriptor_set calls for any shader resource.
        // Therefore it is thread-safe to store the information in the task and then continue it which
        // translates into it being ok that that function is non-const.
        CharacteristicRange const* adding_characteristic_range = shader_resource_plus_characteristic.characteristic_range();
        const_cast<ShaderResourceBase*>(shader_resource)->update_descriptor_set(
            { m_owning_window,
              { m_pipeline_factory_index,
                adding_characteristic_range->characteristic_range_index(),
                adding_characteristic_range->iend() },
              shader_resource_plus_characteristic.fill_index(),
              shader_resource->descriptor_array_size(),
              m_descriptor_set_per_set_index[set_index],
              binding,
              new_descriptor_set->second
            });

        // Find the corresponding SetLayout.
        sorted_descriptor_set_layouts_t::rat sorted_descriptor_set_layouts_r(m_sorted_descriptor_set_layouts);
        auto sorted_descriptor_set_layout =
          std::find_if(sorted_descriptor_set_layouts_r->begin(), sorted_descriptor_set_layouts_r->end(), CompareHint{set_index_hint});
        vk::DescriptorSetLayout descriptor_set_layout = sorted_descriptor_set_layout->handle();
        Dout(dc::shaderresource, "descriptor_set_layout = " << descriptor_set_layout);
        // Store the descriptor set handle corresponding to the used vk::DescriptorSetLayout (SetLayout) and binding number.
        // This unlocks the '{ descriptor_set_layout, binding }' key when the last argument is non-NULL.
        shader_resource->add_set_layout_binding({ descriptor_set_layout, binding }, m_descriptor_set_per_set_index[set_index],
            first_shader_resource ? this : nullptr);
      }
      else
        Dout(dc::shaderresource, "The set_index was not found in set_index_has_frame_resource_pairs.");
    }
  }
}

} // namespace task
