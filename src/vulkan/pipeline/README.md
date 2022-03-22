A `vk::Pipeline` is created by a call to `vulkan::LogicalDevice::create_graphics_pipeline` which returns a `vk::UniquePipeline`.
This library stores the `vk::UniquePipeline` in the `utils::Vector`: `task::PipelineFactory::m_graphics_pipelines` by `vulkan::pipeline::Index`.
Such index therefore only specifies a pipeline uniquely together with its `task::PipelineFactory`.

The `task::PipelineFactory` objects in turn are stored in the `utils::Vector`: `task::SynchronousWindow::m_pipeline_factories` by `PipelineFactoryIndex`.
[ The latter is defined in four places: `vulkan::pipeline::FactoryHandle::PipelineFactoryIndex`, `vulkan::pipeline::Handle::PipelineFactoryIndex`,
`task::PipelineFactory::PipelineFactoryIndex` and `vulkan::Application::PipelineFactoryIndex`. ]
Such index therefore only specifies a pipeline factory uniquely together with its `task::SynchronousWindow`.

The class `vulkan::pipeline::Handle` wraps both indices: `PipelineFactoryIndex` and `vulkan::pipeline::Index` and therefore uniquely identifies a pipeline
when also given the owning `task::SynchronousWindow`.

The normal flow is as follows:

The user class - say `MyWindow` - derived from `task::SynchronousWindow` overrides the virtual function `void create_graphics_pipelines()` and
uses it to create one or more `task::PipelineFactory` objects:

```c
auto pipeline_factory = create_pipeline_factory(*m_pipeline_layout, main_pass.vh_render_pass() COMMA_CWDEBUG_ONLY(true));
```

The returned object is a `vulkan::pipeline::FactoryHandle` which is just a wrapper around a `PipelineFactoryIndex`, but provides two member functions that have to be called next:

```c
pipeline_factory.add_characteristic<MyPipelineCharacteristic1>(this);
pipeline_factory.add_characteristic<MyPipelineCharacteristic2>(this);
...
pipeline_factory.generate(this);
```

where the `MyPipelineCharacteristicX` classes must be derived from `vulkan::pipeline::CharacteristicRange` (or `vulkan::pipeline::Characteristic` when
it doesn't represent a range -- although that class in turn is derived from `vulkan::pipeline::CharacteristicRange` with simply a range size of 1).

When derived from `vulkan::pipeline::Characteristic` the following virtual functions must be overridden:

```c
void initialize(FlatCreateInfo& flat_create_info, task::SynchronousWindow* owning_window) override;
#ifdef CWDEBUG
void print_on(std::ostream& os) const override;         // To print the object to a debug ostream.
#endif
```

When derived from `vulkan::pipeline::CharacteristicRange`, additionally the following virtual function must be overridden:

```c
virtual void fill(FlatCreateInfo& flat_create_info, index_type index) const override;
```

which will be called once for each value `index` in the range that the base class `vulkan::pipeline::CharacteristicRange` was constructed with.

The `initialize` function must register any vectors that this object wants to add data to, which must be declared locally.
For example,

```c
class MyPipelineCharacteristic : public vulkan::pipeline::Characteristic
{
 private:
  std::vector<vk::PipelineShaderStageCreateInfo>     m_pipeline_shader_stage_create_infos;
  std::vector<vk::VertexInputBindingDescription>     m_vertex_input_binding_descriptions;
  std::vector<vk::VertexInputAttributeDescription>   m_vertex_input_attribute_descriptions;
  std::vector<vk::PipelineColorBlendAttachmentState> m_pipeline_color_blend_attachment_states;
  std::vector<vk::DynamicState>                      m_dynamic_states:

  void initialize(vulkan::pipeline::FlatCreateInfo& flat_create_info, task::SynchronousWindow* owning_window) override
  {
    // Register the vectors that we will fill.
    flat_create_info.add(m_pipeline.shader_stage_create_infos());
    flat_create_info.add(m_vertex_input_binding_descriptions);
    flat_create_info.add(m_vertex_input_attribute_descriptions);
    flat_create_info.add(m_pipeline_color_blend_attachment_states);
    flat_create_info.add(m_dynamic_states);

    ...
  }
  ...
};
```

Those are all the possible vectors that can be registered. Don't add vectors that you leave empty.
Different `Characteristic` classes can add vectors of the same type: they will be catenated for
the final pipeline create info.

`initialize` is also intended to fill all static data, that doesn't change when the range index
changes. Then the subsequent calls to `fill` can be used to update just that what changes as function
of the range index.

Adding multiple characteristic ranges results in a product: if one characteristic has a range of size
N and another has a range of size M then N times M pipelines will be created.

Every time a new pipeline is create a synchronous call happens (that is, from the render loop,
before a new frame is drawn) to a member function

```c
void new_pipeline(vulkan::pipeline::Handle pipeline_handle) override;
```

of the `MyWindow` user class (that is derived from `task::SynchronousWindow`).

In this function, the corresponding `task::PipelineFactory` can be retrieved with

```c
boost::intrusive_ptr<task::PipelineFactory> const& pipeline_factory = m_pipeline_factories[pipeline_handle.m_pipeline_factory_index];
```

and the pipeline handle with

```c
vk::Pipeline vh_pipeline = pipeline_factory->vh_pipeline(pipeline_handle.m_pipeline_index);
```

Threading
=========

Each created `task::PipelineFactory` is a task - and thus is run by one thread at a time, while multiple pipeline factories run concurrently.
Each `task::PipelineFactory` has a member

```c
boost::intrusive_ptr<task::PipelineCache const> m_pipeline_cache_task;
```

that points to a `task::PipelineCache` object wrapping a `vk::UniquePipelineCache`. There is exactly one `task::PipelineCache` (and thus pipeline cache)
per unique `vulkan::pipeline::CacheData`, which means - one per `task::PipelineFactory`. In other words, each pipeline factory has its own pipeline
cache object which allows them to run concurrently.

