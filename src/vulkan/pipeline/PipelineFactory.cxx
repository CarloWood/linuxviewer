#include "sys.h"
#include "PipelineFactory.h"
#include "PipelineCache.h"
#include "Handle.h"
#include "SynchronousWindow.h"
#include "SynchronousTask.h"
#include "vk_utils/TaskToTaskDeque.h"
#include "threadsafe/aithreadsafe.h"
#include "utils/at_scope_end.h"

namespace task {

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
    m_owning_window(owning_window), m_pipeline_out(pipeline_out), m_vh_render_pass(vh_render_pass), m_index(vulkan::Application::instance().m_dependent_tasks.add(this)), m_shader_input_data(owning_window)
{
  DoutEntering(dc::statefultask(mSMDebug), "PipelineFactory::PipelineFactory(" << owning_window << ", @" << (void*)&pipeline_out << ", " << vh_render_pass << ")");
}

PipelineFactory::~PipelineFactory()
{
  DoutEntering(dc::statefultask(mSMDebug), "PipelineFactory::~PipelineFactory() [" << this << "]");

  vulkan::Application::instance().m_dependent_tasks.remove(m_index);
}

void PipelineFactory::add_characteristic(boost::intrusive_ptr<vulkan::pipeline::CharacteristicRange> characteristic_range)
{
  characteristic_range->set_owner(this);
  m_characteristics.push_back(std::move(characteristic_range));
}

char const* PipelineFactory::condition_str_impl(condition_type condition) const
{
  switch (condition)
  {
    AI_CASE_RETURN(pipeline_cache_set_up);
    AI_CASE_RETURN(fully_initialized);
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
  if (m_number_of_running_characteristic_tasks.fetch_sub(1, std::memory_order::acq_rel) == 1)
    signal(characteristics_initialized);
}

void PipelineFactory::characteristic_range_filled(vulkan::pipeline::CharacteristicRangeIndex characteristic_range_index)
{
  // Calculate the m_pipeline_index.
  size_t cr_index = characteristic_range_index.get_value();
  m_characteristics[characteristic_range_index]->update(pipeline_index_t::wat{m_pipeline_index}, cr_index, m_range_counters[cr_index], m_range_shift[characteristic_range_index]);
  if (m_number_of_running_characteristic_tasks-- == 1)
    signal(characteristics_filled);
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
        boost::intrusive_ptr<PipelineFactory> parent = this;
        m_number_of_running_characteristic_tasks = m_characteristics.size();
        // Call initialize on each characteristic.
        unsigned int range_shift = 0;
        for (auto i = m_characteristics.ibegin(); i != m_characteristics.iend(); ++i)
        {
          m_range_shift[i] = range_shift;
          range_shift += m_characteristics[i]->range_width();
          m_characteristics[i]->set_flat_create_info(&m_flat_create_info);
          m_characteristics[i]->run(this, characteristics_initialized);
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
        //   while (m_range_counters() < m_characteristics[vulkan::pipeline::CharacteristicRangeIndex{*m_range_counters}]->iend())
        //   {
        if (!(m_range_counters() < m_characteristics[vulkan::pipeline::CharacteristicRangeIndex{*m_range_counters}]->iend()))
        {
          // We did not enter the while loop body.
          run_state = PipelineFactory_bottom_multiloop_for_loop;
          break;
        }
        set_state(PipelineFactory_top_multiloop_while_loop);
        [[fallthrough]];
      case PipelineFactory_top_multiloop_while_loop: // Having multiple for loops inside eachother.
        // We are not at the top of the while loop.
        //
        // Set m_start_of_next_loop to a random magic value to indicate that we are in the inner loop.
        m_start_of_next_loop = std::numeric_limits<int>::max();
        // Required MultiLoop magic.
        if (!m_range_counters.inner_loop())
        {
          // This is not the inner loop; set m_start_of_next_loop to the start value of the next loop.
          m_start_of_next_loop = m_characteristics[vulkan::pipeline::CharacteristicRangeIndex{*m_range_counters + 1}]->ibegin(); // Each loop, one per characteristic, runs from ibegin() till iend().
          // Since the whole reason that we're mimicing the for() while() MultiLoop magic with task states
          // is because we need to be able to leave and enter those loops in the middle of the inner loop,
          // we can not have local state (on the stack); might as well use a goto therefore ;).
          goto multiloop_magic_footer;
        }
        // MultiLoop inner loop body.

        pipeline_index_t::wat{m_pipeline_index}->set_to_zero();

        m_number_of_running_characteristic_tasks = m_characteristics.size();
        // Run over each loop variable (and hence characteristic).
        for (auto i = m_characteristics.ibegin(); i != m_characteristics.iend(); ++i)
        {
          // Do not accidently delete the parent task while still running a child task.
          // Call fill with its current range index.
          m_characteristics[i]->set_fill_index(m_range_counters[i.get_value()]);
          m_characteristics[i]->set_characteristic_range_index(i);
          m_characteristics[i]->signal(vulkan::pipeline::CharacteristicRange::do_fill);
        }
        set_state(PipelineFactory_characteristics_filled);
        wait(characteristics_filled);
        return;
      case PipelineFactory_characteristics_filled:
        //-----------------------------------------------------------------
        // Begin pipeline layout creation

        {
          std::vector<vk::PushConstantRange> const sorted_push_constant_ranges = m_flat_create_info.get_sorted_push_constant_ranges();

          // Realize (create or get from cache) the pipeline layout and return a suitable SetBindingMap.
          m_vh_pipeline_layout = m_owning_window->logical_device()->realize_pipeline_layout(
              m_flat_create_info.get_realized_descriptor_set_layouts(), m_set_binding_map, sorted_push_constant_ranges);
        }

        // Now that we have initialized m_set_binding_map, do the callbacks that need it.
        m_flat_create_info.do_set_binding_map_callbacks(m_set_binding_map);

        if (m_shader_input_data.sort_required_shader_resources_list())
        {
          // No need to call PipelineFactory_create_shader_resources: there are no shader resources that need to be created.
          run_state = PipelineFactory_initialize_shader_resources_per_set_index;
          set_state(PipelineFactory_initialize_shader_resources_per_set_index);
          break;
        }
        set_state(PipelineFactory_create_shader_resources);
        [[fallthrough]];
      case PipelineFactory_create_shader_resources:
        if (!m_shader_input_data.handle_shader_resource_creation_requests(this, m_owning_window, m_set_binding_map))
        {
          //PipelineFactory
          wait(obtained_create_lock);
          return;
        }
        set_state(PipelineFactory_initialize_shader_resources_per_set_index);
        [[fallthrough]];
      case PipelineFactory_initialize_shader_resources_per_set_index:
        m_have_lock = false;
        m_shader_input_data.initialize_shader_resources_per_set_index(m_set_binding_map);
        set_state(PipelineFactory_update_missing_descriptor_sets);
        [[fallthrough]];
      case PipelineFactory_update_missing_descriptor_sets:
        if (!m_shader_input_data.update_missing_descriptor_sets(this, m_owning_window, m_set_binding_map, m_have_lock))
        {
          m_have_lock = true;   // Next time we're woken up (by signal obtained_set_layout_binding_lock) we'll have the lock.
          wait(obtained_set_layout_binding_lock);
          return;
        }

        // End pipeline layout creation
        //-----------------------------------------------------------------

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
          m_move_new_pipelines_synchronously->have_new_datum({vulkan::Pipeline{m_vh_pipeline_layout, {m_pipeline_factory_index, *pipeline_index_t::rat{m_pipeline_index}}, m_shader_input_data.descriptor_set_per_set_index(), m_owning_window->max_number_of_frame_resources()
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
        if (m_range_counters() < m_characteristics[vulkan::pipeline::CharacteristicRangeIndex{*m_range_counters}]->iend())
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
        m_move_new_pipelines_synchronously->set_producer_finished();
        finish();
        return;
    }
  }
}

} // namespace task
