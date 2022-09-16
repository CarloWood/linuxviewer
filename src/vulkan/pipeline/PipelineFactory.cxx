#include "sys.h"
#include "PipelineFactory.h"
#include "PipelineCache.h"
#include "Handle.h"
#include "SynchronousWindow.h"
#include "SynchronousTask.h"
#include "vk_utils/TaskToTaskDeque.h"
#include "threadsafe/aithreadsafe.h"

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
    m_owning_window(owning_window), m_pipeline_out(pipeline_out), m_vh_render_pass(vh_render_pass), m_index(vulkan::Application::instance().m_dependent_tasks.add(this))
{
  DoutEntering(dc::statefultask(mSMDebug), "PipelineFactory::PipelineFactory(" << owning_window << ", @" << (void*)&pipeline_out << ", " << vh_render_pass << ")");
}

PipelineFactory::~PipelineFactory()
{
  DoutEntering(dc::statefultask(mSMDebug), "PipelineFactory::~PipelineFactory() [" << this << "]");

  vulkan::Application::instance().m_dependent_tasks.remove(m_index);
}

void PipelineFactory::add(boost::intrusive_ptr<vulkan::pipeline::CharacteristicRange> characteristic_range)
{
  m_characteristics.push_back(std::move(characteristic_range));
}

char const* PipelineFactory::condition_str_impl(condition_type condition) const
{
  switch (condition)
  {
    AI_CASE_RETURN(pipeline_cache_set_up);
    AI_CASE_RETURN(fully_initialized);
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
    AI_CASE_RETURN(PipelineFactory_generate);
    AI_CASE_RETURN(PipelineFactory_done);
  }
  AI_NEVER_REACHED
}

char const* PipelineFactory::task_name_impl() const
{
  return "PipelineFactory";
}

void PipelineFactory::multiplex_impl(state_type run_state)
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
      break;
    }
    case PipelineFactory_initialize:
      // Start a synchronous task that will be run when this task, that runs asynchronously, created a new pipeline and/or is finished.
      m_move_new_pipelines_synchronously = statefultask::create<synchronous::MoveNewPipelines>(m_owning_window, m_pipeline_factory_index COMMA_CWDEBUG_ONLY(mSMDebug));
      m_move_new_pipelines_synchronously->run();
      // Wait until the user is done adding CharacteristicRange objects and called generate().
      set_state(PipelineFactory_initialized);
      wait(fully_initialized);
      break;
    case PipelineFactory_initialized:
    {
      // Do not use an empty factory - it makes no sense.
      ASSERT(!m_characteristics.empty());
      vulkan::pipeline::Index max_pipeline_index{0};
      // Call initialize on each characteristic.
      for (int i = 0; i < m_characteristics.size(); ++i)
      {
        m_characteristics[i]->initialize(m_flat_create_info, m_owning_window);
        m_characteristics[i]->update(max_pipeline_index, m_characteristics[i]->iend() - 1);
      }
      // max_pipeline_index is now equal to the maximum value that a pipeline_index can be.
//FIXME: is max_pipeline_index still needed?      m_graphics_pipelines.resize(max_pipeline_index.get_value() + 1);

      // Start as many for loops as there are characteristics.
      m_range_counters.initialize(m_characteristics.size(), m_characteristics[0]->ibegin());
      // Enter the multi-loop.
      set_state(PipelineFactory_generate);
      [[fallthrough]];
    }
    case PipelineFactory_generate:
      // Each time we enter from the top, m_range_counters.finished() should be false.
      // It is therefore the same as entering at the bottom of the while loop at *).
      for (; !m_range_counters.finished(); m_range_counters.next_loop())                // This is just MultiLoop magic; these two lines result in
        while (m_range_counters() < m_characteristics[*m_range_counters]->iend())       // having multiple for loops inside eachother.
        {
          // Set b to a random magic value to indicate that we are in the inner loop.
          int b = std::numeric_limits<int>::max();
          // Required MultiLoop magic.
          if (m_range_counters.inner_loop())
          {
            // MultiLoop inner loop body.

            vulkan::pipeline::Index pipeline_index{0};

            // Run over each loop variable (and hence characteristic).
            for (int i = 0; i < m_characteristics.size(); ++i)
            {
              // Call fill with its current range index.
              m_characteristics[i]->fill(m_flat_create_info, m_range_counters[i]);
              // Calculate the pipeline_index.
              m_characteristics[i]->update(pipeline_index, m_range_counters[i]);
            }

            // Merge the results of all characteristics into local vectors.
            std::vector<vk::VertexInputBindingDescription>     const vertex_input_binding_descriptions      = m_flat_create_info.get_vertex_input_binding_descriptions();
            std::vector<vk::VertexInputAttributeDescription>   const vertex_input_attribute_descriptions    = m_flat_create_info.get_vertex_input_attribute_descriptions();
            std::vector<vk::PipelineShaderStageCreateInfo>     const pipeline_shader_stage_create_infos     = m_flat_create_info.get_pipeline_shader_stage_create_infos();
            std::vector<vk::PipelineColorBlendAttachmentState> const pipeline_color_blend_attachment_states = m_flat_create_info.get_pipeline_color_blend_attachment_states();
            std::vector<vk::DynamicState>                      const dynamic_state                          = m_flat_create_info.get_dynamic_states();
            vk::PipelineLayout vh_pipeline_layout;
            {
              vulkan::pipeline::ShaderInputData::sorted_set_layouts_container_t const& realized_descriptor_set_layouts = m_flat_create_info.get_realized_descriptor_set_layouts();
              std::vector<vk::PushConstantRange>               const sorted_push_constant_ranges            = m_flat_create_info.get_sorted_push_constant_ranges();

              //-----------------------------------------------------------------
              // Begin pipeline layout creation

              std::map<vulkan::descriptor::SetBinding, vulkan::descriptor::SetBinding> set_binding_map;
              vh_pipeline_layout = m_owning_window->logical_device()->try_emplace_pipeline_layout(realized_descriptor_set_layouts, set_binding_map, sorted_push_constant_ranges);
              Dout(dc::always, "set_binding_map = " << set_binding_map);

              // End pipeline layout creation
              //-----------------------------------------------------------------
              // Bug in this library: the layout must be created.
              ASSERT(vh_pipeline_layout);
            }

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
              .layout = vh_pipeline_layout,
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
            Dout(dc::finish, " --> pipeline::Index " << pipeline_index);
#endif

            // FIXME: We stop here because continueing causes "Lost Device".
            static std::atomic_int cnt{0};
            int prev_cnt = cnt.fetch_add(1);
            ASSERT(prev_cnt < 2);
            if (prev_cnt == 0)
            {
              std::this_thread::sleep_for(std::chrono::milliseconds(100));
              DoutFatal(dc::core, "Reached create_graphics_pipeline - after sleep!");
            }
            DoutFatal(dc::core, "Reached create_graphics_pipeline!");

            // Create and then store the graphics pipeline.
            vk::UniquePipeline pipeline = m_owning_window->logical_device()->create_graphics_pipeline(m_pipeline_cache_task->vh_pipeline_cache(), pipeline_create_info
                COMMA_CWDEBUG_ONLY(m_owning_window->debug_name_prefix("pipeline")));

            // Inform the SynchronousWindow.
            m_move_new_pipelines_synchronously->have_new_datum({vulkan::Pipeline{vh_pipeline_layout, {m_pipeline_factory_index, pipeline_index}}, std::move(pipeline)});

            // End of MultiLoop inner loop.
          }
          else
            // This is not the inner loop; set b to the start value of the next loop.
            b = m_characteristics[*m_range_counters + 1]->ibegin();                     // Each loop, one per characteristic, runs from ibegin() till iend().
          // Required MultiLoop magic.
          m_range_counters.start_next_loop_at(b);
          // If this is true then we're at the end of the inner loop and want to yield.
          if (b == std::numeric_limits<int>::max() && !m_range_counters.finished())
          {
            yield();
            return;     // Yield and then continue here --.
          }             //                                |
          // *) <-- actual entry point of this state <----'
        }
      set_state(PipelineFactory_done);
      [[fallthrough]];
    case PipelineFactory_done:
      m_move_new_pipelines_synchronously->set_producer_finished();
      finish();
      break;
  }
}

} // namespace task
