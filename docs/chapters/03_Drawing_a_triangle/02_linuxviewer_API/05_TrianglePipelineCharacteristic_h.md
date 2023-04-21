---
layout: chapter
title: Linuxviewer API
toc_title: TrianglePipelineCharacteristic.h
utterance_id: lv_API_TrianglePipelineCharacteristic_h_draft
---
{% assign drawing_a_triangle = site.baseurl | append: '/chapters/03_Drawing_a_triangle' -%}

## {% include link_to_github folder="examples/hello_triangle/" name="TrianglePipelineCharacteristic.h" %}

As discussed [before]({{ drawing_a_triangle }}/01_The_Vulkan_API.html#page_vkcommandbuffer)
an application might need many pipelines.

Linuxviewer allows the user to create pipelines in the background with "pipeline factories".
Each factory uses one or more "pipeline characteristics", classes that describe a particular
pipeline characteristic. By combining the characteristics as building blocks to build pipeline
factories it is possible to make use of code re-use.

Our 'Hello Triangle' application has a single pipeline, and thus a single pipeline factory
that creates it, which uses a single characteristic `TrianglePipelineCharacteristic` that
describes the whole pipeline.

```cpp
#pragma once

#include "Window.h"
#include <vulkan/pipeline/Characteristic.h>
#include <vulkan/pipeline/AddVertexShader.h>
#include <vulkan/pipeline/AddFragmentShader.h>
#include "debug.h"

class TrianglePipelineCharacteristic :
    public vulkan::pipeline::Characteristic,
    public vulkan::pipeline::AddVertexShader,
    public vulkan::pipeline::AddFragmentShader
{
```

This `TrianglePipelineCharacteristic` is a "pipeline characteristics" and therefore
derived from `vulkan::pipeline::Characteristic`. Furthermore, our pipeline
needs a vertex shader and a fragment shader; therefore we also derive from
`vulkan::pipeline::AddVertexShader` and `vulkan::pipeline::AddFragmentShader`
respectively.

Note that a single pipeline characteristic doesn't necessarily need to describe all stages, or even a single stage, of the pipeline.
For example, we could have used three Characteristics; one that describes the vertex shader, another that describes the fragment shader
and one describing things not related to any shader.

<a id="dynamic-states" />
<div class="extended-hr"><hr /></div>
```cpp
 private:
  std::vector<vk::DynamicState> m_dynamic_states = {
    vk::DynamicState::eViewport,
    vk::DynamicState::eScissor
  };
```

As described [before]({{ drawing_a_triangle }}/01_The_Vulkan_API.html#dynamic-state) some
state that normally is part of a pipeline can also be configured dynamically; every frame. The downside is
that that takes a few clock cycles extra per frame, but on the other hand it can mean that we need considerably less
different pipelines.

This vector lists the state that we want to be dynamic; that we commit to writing every frame into the
[command buffer]({{ drawing_a_triangle }}/01_The_Vulkan_API.html#dynamic-recording)
that describes how to render the next frame.

It is pretty common to make the [viewport and scissors](https://stackoverflow.com/a/42506710/1487069) dynamic.
We added them here to show how this is done, even though in this application we won't change their values between frames.

<div class="extended-hr"><hr /></div>
```cpp
  std::vector<vk::PipelineColorBlendAttachmentState> m_pipeline_color_blend_attachment_states = {
    vk_defaults::PipelineColorBlendAttachmentState{}
  };
```

A lot of pipeline state that will be baked into the
[VkPipeline]({{ drawing_a_triangle }}/01_The_Vulkan_API.html#page_vkpipeline)
is transfered using a `std::vector` to a `vulkan::FlatCreateInfo` object. Even if we only need a single element.

The pipeline state that tells a pipeline how to blend color attachments is a required state,
even though we are using a default for it here. Alternatively, one can `push_back`
elements onto the vector in the `initialize()` member function (see below).

<div class="extended-hr"><hr /></div>
```cpp
 public:
  TrianglePipelineCharacteristic(vulkan::task::SynchronousWindow const* owning_window COMMA_CWDEBUG_ONLY(bool debug)) :
    vulkan::pipeline::Characteristic(owning_window COMMA_CWDEBUG_ONLY(debug))
  {
    // Required when we don't have vertex buffers.
    m_use_vertex_buffers = false;
  }
```

Since we're not using any vertex buffers--- the coordinates of the triangle's corners are hardcoded in its vertex shader
as is the tradition for a HelloTriangle application--- we need to implement the constructor, passing its arguments to
the base class, so that we can set the base class variable `m_use_vertex_buffers` to false.

{% capture tip_content %}
If we don't do this we'd run into an `ASSERT`,

{% capture debug_output %}
ThreadPool02    COREDUMP      : |   /.../pipeline/AddVertexShader.cxx:38: virtual void vulkan::pipeline::AddVertexShader::copy_shader_variables(): Assertion 'm_current_vertex_buffers_object' failed.
{% endcapture %}{% include debug_output.html %}

with a comment that reads

```cpp
// Call AddVertexShader::add_vertex_input_bindings with a vulkan::VertexBuffers
// object from the initialization state of the Characteristic responsible for
// the vertex shader. If you are not using any VertexBuffers, then set
// m_use_vertex_buffers to false in the constructor of the pipeline factory
// characteristic derived from this class.
ASSERT(m_current_vertex_buffers_object);
```

which hopefully speaks for itself (how to add a vertex buffer will be handled later on).

Whenever you run into an assert make sure to look at the comment above it in the code!
{% endcapture %}{% include lv_note.html %}

Setting this boolean to false prevents `AddVertexShader` to register its baked in `m_vertex_input_binding_descriptions`
and `m_vertex_input_attribute_descriptions` vectors with the `FlatCreateInfo`. Baked in because normally *everyone*
will use vertex buffers--- *except* when you are doing the Hello Triangle thing.

Whenever you register a vector with the `FlatCreateInfo` you are required to fill it with data: if no data is needed,
don't register it. This is done to easily detect when someone forgot to fill a vector with data.

<div class="extended-hr"><hr /></div>
```cpp
 protected:
  void initialize() final
  {
    Window const* window = static_cast<Window const*>(m_owning_window);
```

If a pipeline characteristic is derived from `vulkan::Characteristic`,
as opposed to from `vulkan::CharacteristicRange`, then `initialize()`
is the only virtual function that needs to be overridden:
all initialization can, and is, done in a single function.

Typically the Characteristic will need configuration that is defined in the
Window class that will use the pipeline(s) created with this Characteristic.
Therefore we obtain a pointer to the window object.

{% capture tip_content %}
This means that there is a one-on-one relationship between a Window and
a pipeline characteristic. Typically you should therefore define the
Characteristic as a nested class of the Window class.

That way a Characteristic automatically becomes a friend of the Window
and will be able to access its private members through the above `window`
pointer.

If you need a certain Characteristic for multiple Windows then you should
make a WindowBase class that contains the Characteristic and all members
that that needs access to, and then use that base class for the different
Window types that need pipelines with that characteristic.
{% endcapture %}{% include lv_important.html %}

Next, we register all pipeline state that this Characteristic will provide.
This doesn't need to be *all* state of the pipeline, as this just one
characteristic of a pipeline; but in the case of the HelloTriangle application
we only have a single characteristic, so this `initialize` function is
going to provide all state.

```cpp
    // Register the vectors that we will fill.
    add_to_flat_create_info(m_dynamic_states);
    add_to_flat_create_info(m_pipeline_color_blend_attachment_states);
```

In this case these two vectors were already initialized (see above),
but that is not required. Here we merely state that we will fill these
vectors with create info that is required to create a pipeline. The
vectors can also be filled afterwards. Keep in mind that if we leave
a vector empty after adding them to the `FlatCreateInfo` like this,
then the engine will assert, assuming you forgot to fill it.

{% capture tip_content %}
The following vector types can be registered with the `FlatCreateInfo` using `add_to_flat_create_info`:

* `std::vector<vk::DynamicState>`
<span class="bullet_point_description">Up to 86 different things can be marked as "dynamic state", enumerated by
the values of `vk::DynamicState` ({% include link_to_spec_1.3 name="VkDynamicState" %}).
In our case we only used `vk::DynamicState::eViewport` and `vk::DynamicState::eScissor`, see `m_dynamic_states` above.</span>

* `std::vector<vk::PipelineColorBlendAttachmentState>`
<span class="bullet_point_description">
This mandatory pipeline state specifies how to blend color attachments.
See the documentation of `vk::PipelineColorBlendAttachmentState` ({% include link_to_spec_1.3 name="VkPipelineColorBlendAttachmentState" %})
for a description.
In our case we just used the default defined in `vk_defaults`, see `m_pipeline_color_blend_attachment_states` above.</span>

* `std::vector<vk::VertexInputBindingDescription>`
<span class="bullet_point_description">This is already taken care of by deriving from `vulkan::AddVertexShader`.</span>

* `std::vector<vk::VertexInputAttributeDescription>`
<span class="bullet_point_description">This is already taken care of by deriving from `vulkan::AddVertexShader`.</span>

* `std::vector<vk::PipelineShaderStageCreateInfo>`
<span class="bullet_point_description">This is already taken care of by `vulkan::AddShaderStage`, a virtual base class of
`vulkan::AddVertexShader` and `vulkan::AddFragmentShader`.</span>

* `std::vector<vulkan::PushConstantRange>`
<span class="bullet_point_description">This can be used to specify which stages will use what part of the push constant block.
Although linuxviewer supports push constants (derive the pipeline characteristic from `vulkan::AddPushConstant`),
there is no automated support for this yet. It seems likely that automation should be added in the future though.</span>
{% endcapture %}{% include lv_note.html %}

{% capture tip_content %}
If `m_dynamic_states` and/or `m_pipeline_color_blend_attachment_states` would still have been empty
then this point would have been the appropriate place to fill them. For example, by doing
```cpp
    m_pipeline_color_blend_attachment_states.
        push_back(vk_defaults::PipelineColorBlendAttachmentState{});
```
here.
{% endcapture %}{% include info_note.html %}

<div class="extended-hr"><hr /></div>
```cpp
    // Add default topology.
    m_flat_create_info->m_pipeline_input_assembly_state_create_info.topology = vk::PrimitiveTopology::eTriangleList;
```

Certain members of the `FlatCreateInfo` are public and can be changed directly
(but should, for any given pipeline factory, only be changed from a single Characteristic).
Above we change the `topology` member of the public
`vk::PipelineInputAssemblyStateCreateInfo` ({% include link_to_spec_1.3 name="VkPipelineInputAssemblyStateCreateInfo" %}) member
from its default of `vk::PrimitiveTopology::eLineList` to `eTriangleList`. After all, we want to draw a filled, colorful triangle,
not show three lines.

{% capture tip_content %}
The following non-vector types of `FlatCreateInfo` can be accessed directly:

* `vk::PipelineInputAssemblyStateCreateInfo m_pipeline_input_assembly_state_create_info`
<span class="bullet_point_description">influences the configuration of the input assembly stage,
determining how vertex data is assembled into primitives such as points, lines, or triangles.
See the Vulkan API documentation of {% include link_to_spec_1.3 name="VkPipelineInputAssemblyStateCreateInfo" %} for details.
The default is to draw lines.</span>

* `vk::PipelineViewportStateCreateInfo m_viewport_state_create_info`
<span class="bullet_point_description">influences the configuration of the viewport and scissor test stages,
defining the transformation of vertex positions to screen-space coordinates and specifying the rectangular region for scissor testing.
See the Vulkan API documentation of {% include link_to_spec_1.3 name="VkPipelineViewportStateCreateInfo" %} for details.</span>

* `vk::PipelineRasterizationStateCreateInfo m_rasterization_state_create_info`
<span class="bullet_point_description">influences the configuration of the rasterization stage,
determining how primitives are converted into fragments, as well as controlling polygon modes, line width, and culling operations.
See the Vulkan API documentation of {% include link_to_spec_1.3 name="VkPipelineRasterizationStateCreateInfo" %} for details.</span>

* `vk::PipelineMultisampleStateCreateInfo m_multisample_state_create_info`
<span class="bullet_point_description">influences the configuration of the multisampling stage,
determining the multisampling parameters such as sample count, sample shading,
and alpha-to-coverage settings for anti-aliasing and smoother rendering.
See the Vulkan API documentation of {% include link_to_spec_1.3 name="VkPipelineMultisampleStateCreateInfo" %} for details.</span>

* `vk::PipelineDepthStencilStateCreateInfo m_depth_stencil_state_create_info`
<span class="bullet_point_description">influences the configuration of the depth and stencil test stages,
specifying the depth and stencil test operations, comparison functions, and writing masks for enhanced control
over depth and stencil buffer interactions.
See the Vulkan API documentation of {% include link_to_spec_1.3 name="VkPipelineDepthStencilStateCreateInfo" %} for details.</span>

* `vk::PipelineColorBlendStateCreateInfo m_color_blend_state_create_info`
<span class="bullet_point_description">influences the configuration of the color blending stage,
determining how the output color from the fragment shader is combined with the existing framebuffer color,
and setting up blend operations, factors, and write masks for multiple render targets.
See the Vulkan API documentation of {% include link_to_spec_1.3 name="VkPipelineColorBlendStateCreateInfo" %} for details.</span>
{% endcapture %}{% include lv_note.html %}

<a id="use_shaders" />
<div class="extended-hr"><hr /></div>
```cpp
    // Register the two shaders that we use.
    add_shader(window->m_shader_vert);
    add_shader(window->m_shader_frag);
  }
```
All shaders that are used by this pipeline must be added with a call to `add_shader`.
`M_shader_vert` and `m_shader_frag`, with type `ShaderIndex`, are registered in
[`Window::register_shader_templates`]({{ page.path | remove:page.name | relative_url }}/07_Window_cxx.html#register-shader-templates).

<div class="extended-hr"><hr /></div>
```cpp
 public:
#ifdef CWDEBUG
  // Implement pure virtual function from the base class that allows us to write this class as debug output.
  void print_on(std::ostream& os) const override
  {
    // Just print the type, not any contents.
    os << "{ (FragmentPipelineCharacteristicRange*)" << this << " }";
  }
#endif
};
```
This causes the debug output that attempts to print this Characteristic to show the most derived type
`FragmentPipelineCharacteristicRange` and the value of the `this` pointer.
The detailed content of the object seems less important in this case.

{% capture tip_content %}
All debug output is written to an ostream, allowing easy printing of any (user) type.
However, without the use of custom ostream manipulators (for example as provided by
[<span class="command">iomanip.h</span>](https://github.com/CarloWood/ai-utils/blob/master/iomanip.h)
of the [<span class="command">utils</span>](https://github.com/CarloWood/ai-utils) git submodule),
the format in which objects are printed is fixed.
This more or less limits the use of writing object (to an ostream) to only a single purpose.

We have chosen this purpose to be debug output; all `aicxx` code, and the linuxviewer Vulkan engine itself
consider anything that is written to an `ostream` to be debug output; the format for printing objects to a
`std::ostream` is therefore exclusively aimed at debugging.

Apart from the limitation of a more or less fixed format, C++ has another perceived limitation in handling
ostream output, as the implementation of the `operator<<` for custom classes must be done as a *non-member function*,
which goes against the OO paradigm.

This is why `aicxx`-based code relies on the principle that all debug output for any object must be implemented
using the member function `void print_on(std::ostream& os) const`.

To make this work one has to include the header `<utils/has_print_on.h>` which contains the ostream `operator<<` that will be used.
This header is already included by `"debug.h"` of the main project.
The only extra requirement is that, if the class that has the `print_on` member function resides in a namespace,
then inside that same namespace one must add `using utils::has_print_on::operator<<;`
(see also the `EXAMPLE_CODE` in [<span class="command">has_print_on.h</span>](https://github.com/CarloWood/ai-utils/blob/master/has_print_on.h)).
{% endcapture %}{% include lv_note.html %}

