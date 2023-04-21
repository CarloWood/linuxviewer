---
layout: chapter
title: Linuxviewer API
utterance_id: linuxviewer_api_draft
---
* TOC
{:toc}

## Introduction ##

Drawing our first triangle is the *Hello World* of Vulkan.

Linuxviewer comes with an [example application](https://github.com/CarloWood/linuxviewer/tree/master/src/examples/hello_triangle)
that opens a window with a size of 400x400 and draws a triangle in it.
<span class="command">Hello_triangle</span> is made up of the following files:

* <span class="command">HelloTriangle.cxx, HelloTriangle.h</span>:
The application definition. <span class="command">HelloTriangle.cxx</span> contains<br>`int main(int argc, char* argv[])`
and initialization that is required for every linuxviewer application. <span class="command">HelloTriangle.h</span> defines
the application class and contains more configuration by overriding a few virtual functions.

* <span class="command">Window.cxx, Window.h</span>: The window definition.
Together these define almost everything related to the Vulkan rendering; after all, the triangle that we want
to draw has to appear inside this window. We could have another window with something else, so the "triangle"
is not really related to the Application.

* <span class="command">LogicalDevice.h</span>: The definition of the [logical device](#page_vkdevice).
The required Vulkan features and queues are specified here, again by overriding virtual functions of the base class.

* <span class="command">TrianglePipelineCharacteristic.h</span>: Most of the pipeline configuration.
The class `TrianglePipelineCharacteristic` specifies a characteristic of the pipeline. It is passed
to a pipeline factory that then creates the actual pipeline. Since we only have one characteristic
in this case I should say it specifies all characteristic.

{% capture tip_content %}
Each window in linuxviewer has its own target surface, swapchain and render loop.
Windows can run at different FPS, even when one window is a child window of another.
For example, one can create an input field that runs at 60 FPS while the rest
of the window runs at 4 FPS, still maintaining a smooth input experience.
{% endcapture %}{% include lv_note.html %}

Below we will go over the source code of each file and add an explaination of the code.

## {% include link_to_github folder="examples/hello_triangle/" name="HelloTriangle.cxx" %}

```cpp
#include "sys.h"
```

As all git submodules use [libcwd](http://carlowood.github.io/libcwd/index.html) for debugging output,
it is required to begin *every* source file (that use libcwd macros) with this line.

This will include the <span class="command">sys.h</span> header file from the following locations, in order of priority:
first, the compiler will look for the file in the root of the project; if it doesn't exist there, it will search
the root of the build directory; and finally, if neither of those locations has the file,
it will use the [<span class="command">sys.h</span>](https://github.com/CarloWood/cwds/blob/master/sys.h) file from
[<span class="command">cwds</span>](https://github.com/CarloWood/cwds).

<div class="extended-hr"><hr /></div>
```cpp
#include "HelloTriangle.h"
#include "LogicalDevice.h"
#include "Window.h"
```

These header respectively define the Application class, the LogicalDevice and the Window that we want to open.

<div class="extended-hr"><hr /></div>
```cpp
#include "debug.h"
```

This header has to be included every time a file uses one or more <span class="command">libcwd</span> macros.
Note that it is also required when compiling the application without debug support (i.e. `Release` mode)
because in that case all those macros need to be defined as empty (or you'd get error about undefined "functions").

Since other headers likely already included a header from one of the many git submodules, or linuxviewer,
it is extremely likely that <span class="command">debug.h</span> was *already* included, but it is a good
habbit to include it anyway whenever a file uses one of the <span class="command">libcwd</span> macros.
In the case of <span class="command">HelloTriangle.cxx</span> we use `Debug`, `Dout` and `DoutFatal`.

<div class="extended-hr"><hr /></div>
```cpp
#ifdef CWDEBUG
#include <utils/debug_ostream_operators.h> // Required to print AIAlert::Error to an ostream.
#endif
```

In this case we put `#ifdef CWDEBUG ... #endif` around the `#include`
to avoid including it when we are not compiling in debug mode; in particular, when we don't have
debug output (`Dout` et al are empty).

<div class="extended-hr"><hr /></div>
```cpp
// Required header to be included below all other headers (that might include vulkan headers).
#include <vulkan/lv_inline_definitions.h>
```

The complexity of the linuxviewer headers requires the use of separate files for the definition of inline functions,
in order to avoid circular header dependencies.
The downside is that those need to be included in every compilation unit.

To make this easier, the header file <span class="command">lv_inline_definitions.h</span> can be used,
below all other includes, to automatically include the <span class="command">.inl.h</span> headers
that are required and weren't included already.

{% capture tip_content %}
Note that if you need inlined functions in a *header* then it is better to include the specific
<span class="command">.inl.h</span> header, below any other headers, required for those functions
rather than <span class="command">lv_inline_definitions.h</span>.

In all cases, these <span class="command">.inl.h</span> headers must be included *after* all
linuxviewer headers have already been included, which unfortunately means that headers,
of your own project, that need <span class="command">.inl.h</span> headers must be
included after the linuxviewer headers.
{% endcapture %}{% include lv_important.html %}

<div class="extended-hr"><hr /></div>
```cpp
int main(int argc, char* argv[])
{
  Debug(NAMESPACE_DEBUG::init());
```

The line `Debug(NAMESPACE_DEBUG::init());` is required to initialize [cwds](https://github.com/CarloWood/cwds),
(the (lib)**cwd** **s**(upport) git submodule) and through that also
[<span class="command">libcwd</span>](http://carlowood.github.io/libcwd/index.html) itself.

<div class="extended-hr"><hr /></div>
```cpp
  Dout(dc::notice, "Entering main()");
```

This prints debug output, to `std::cout` because
we didn't [change the default debug ostream](http://carlowood.github.io/libcwd/reference-manual/group__group__destination.html):

{% capture debug_output %}
NOTICE        : Entering main()
{% endcapture %}{% include debug_output.html %}

and will in fact be the very first debug output that the application print
(when using <span class="command">silent = on</span> in <span class="command">.libcwdrc</span>)
because linuxviewer uses no global variables. Global variables are evil.
All initialization will happen *after* entering main.

<div class="extended-hr"><hr /></div>
```cpp
  try
  {
    // Create the application object.
    HelloTriangle application;
```

The whole application is put inside a `try ... catch` block, including
the construction of the Application object. This is the safest way to
assure that, if there is an exception thrown that isn't caught by the
vulkan engine code, we'll catch and display it here as a last resort.

In general, irrecoverable errors are thrown as `AIAlert::Error` objects
(part of the [utils](https://github.com/CarloWood/utils) git submodule)
which collect detailed information from the whole call stack involved in
the error, by being caught added to and re-thrown multiple times, if needed.

The main idea of `AIAlert::Error` exceptions is to end up in a popup
window that informs the *user* that they tried to do something that
is not possible, and give detailed information about why it is not
possible (as opposed to the usual "Something went wrong, please try again").

Therefore, normally these exceptions should be caught as part of
user actions (i.e. they click on a button) and the application as a whole
should not crash, but recover "as if" the user didn't click that button.

In the case of our HelloTriangle however, there are no user actions and
any such exception *will* be fatal, so all we can do is print the error
and terminate. In general this will be the case for all exceptions that
reach `main`.

<div class="extended-hr"><hr /></div>
```cpp
    // Initialize application using the virtual functions of FrameResourcesCount.
    application.initialize(argc, argv);
```

Here is where possible commandline arguments are processed and stored
in member variables of the application object.

Among others, this causes the virtual function
`parse_command_line_parameters(int argc, char* argv[])` to be called.
By overriding this virtual function in the user class (`HelloTriangle` in this case)
the user can add their own decoding and handling of command line parameters.

There are several other [virtual functions](#virtual_functions) of `vulkan::Application`, the
base call of `HelloTriangle`, that can be overridden.

<div class="extended-hr"><hr /></div>
```cpp
    // Create a window.
    auto root_window = application.create_root_window<vulkan::WindowEvents, Window>({400, 400}, LogicalDevice::root_window_request_cookie);
```

This creates a "root" window. This is not the same as the X11 root window that spans
the whole monitor, it just means that it is a normal window, as opposed to a child window (of another window).

The arguments passed are the initial size and a `int` value that is used while assigning
[command queues]({{ page.path | remove:page.name | relative_url }}/01_The_Vulkan_API.html#page_vkdevice)

<div class="extended-hr"><hr /></div>
```cpp
    // Create a logical device that supports presenting to root_window.
    auto logical_device = application.create_logical_device(std::make_unique<LogicalDevice>(), std::move(root_window));
```

This figures out which [Physical Device]({{ page.path | remove:page.name | relative_url }}/01_The_Vulkan_API.html#page_vkphysicaldevice)
to use, by looking for the most powerful GPU that supports the requested `LogicalDevice` (as defined in `LogicalDevice.h`), and is
able to display the requested window. It then creates and returns a smart pointer to the created
[<span class="command">VkDevice</span>]({{ page.path | remove:page.name | relative_url }}/01_The_Vulkan_API.html#page_vkdevice)
in the form of a `boost::intrusive_ptr<task::LogicalDevice>`; this is not really the <span class="command">VkDevice</span>,
it is an [aistatefultask](https://github.com/CarloWood/ai-statefultask) 'task' object that is will create the device,
possibly in another thread (it's complicated). Such tasks are [always](http://carlowood.github.io/statefultask/lifecycle.html)
returned as `boost::intrusive_ptr`s. Note that going out of scope means automatic destruction of the device task, and consequently,
the window because the `LogicalDevice` owns this window now (it was moved there), and the logical device.

<div class="extended-hr"><hr /></div>
```cpp
    // Run the application.
    application.run();
```

Now that everything has been "configured", really begin.
This does too much to explain here and even if it wasn't--- you don't want to know.
In fact, this opens the window and shows a triangle.

<img src="{{ '/assets/LV_HelloTriangle.png' | relative_url }}" alt="Hello Triangle" id="img_hellotriangle_large" />

<div class="extended-hr"><hr /></div>
```cpp
  }
  catch (AIAlert::Error const& error)
  {
    // Application terminated with an error.
    Dout(dc::warning, error << ", caught in HelloTriangle.cxx");
  }
#ifndef CWDEBUG // Commented out so we can see in gdb where an exception is thrown from.
  catch (std::exception& exception)
  {
    DoutFatal(dc::core, "std::exception: " << exception.what() << " caught in HelloTriangle.cxx");
  }
#endif
```

Poor mans error handling. Just print caught exceptions instead of letting them go to waste. Then exit main.
If the (last) window is closed `application.run()` runs normally and we also leave main, of course.

<div class="extended-hr"><hr /></div>
```cpp
  Dout(dc::notice, "Leaving main()");
}
```

But not before printing that we do so! Note this is not the last debug output line because linuxviewer
*does* use a few singletons that are destructed after `main` as a result of having `static` member objects.

## {% include link_to_github folder="examples/hello_triangle/" name="HelloTriangle.h" %}

```cpp
#pragma once
```

It's a header...

<div class="extended-hr"><hr /></div>
```cpp
#include <vulkan/Application.h>
```

Needed because `HelloTriangle` is derived from `vulkan::Application`.

```cpp
class HelloTriangle : public vulkan::Application
{
 public:
  std::u8string application_name() const override
  {
    return u8"Hello Triangle";
  }
};
```

All the default virtual functions of `vulkan::Application` are good for this "hello world" application,
except that default `"Application Name"`, noone can live with that.

{% capture tip_content %}
The following virtual functions of `vulkan::Application` allow further configuration of the application:

* `std::string default_display_name() const`
<span class="bullet_point_description">The default <span class="command">DISPLAY</span> name to use (can be overridden by parse_command_line_parameters) (default: `":0"`).</span>

* `int thread_pool_number_of_worker_threads() const`
<span class="bullet_point_description">The number of worker threads to use for the thread pool (default: `vulkan::Application::default_number_of_threads` which is 8).</span>

* `int thread_pool_queue_capacity(QueuePriority priority) const`
<span class="bullet_point_description">The size of the thread pool queues (default: equal to the number of worker threads).</span>

* `int thread_pool_reserved_threads(QueuePriority priority) const`
<span class="bullet_point_description">The number of reserved threads for each queue (default: `vulkan::Application::default_reserved_threads` which is 1).</span>

* `void prepare_instance_info(vulkan::InstanceCreateInfo& instance_create_info) const`
<span class="bullet_point_description">Override this function to add Instance layers and/or extensions.</span>

* `std::u8string application_name() const`<br>
<span class="bullet_point_description">Override this function to change the default ApplicationInfo values
(default: `"Application Name"`).</span>

* `uint32_t application_version() const`<br>
<span class="bullet_point_description">Override this function to change the default application version.
The result should be a value returned by vk_utils::encode_version (default: `vk_utils::encode_version(0, 0, 0)`).</span>
{% endcapture %}{% include lv_note.html %}

## {% include link_to_github folder="examples/hello_triangle/" name="LogicalDevice.h" %}

```cpp
#pragma once

#include <vulkan/LogicalDevice.h>

class LogicalDevice : public vulkan::LogicalDevice
{
```

Also `vulkan::LogicalDevice` has virtual functions that allow the user to configure it.

{% capture tip_content %}
The following virtual functions of `vulkan::LogicalDevice` allow further configuration of the device:

* <pre>void prepare_physical_device_features(
    vk::PhysicalDeviceFeatures& features10,
    vk::PhysicalDeviceVulkan11Features& features11,
    vk::PhysicalDeviceVulkan12Features& features12,
    vk::PhysicalDeviceVulkan13Features& features13) const</pre>
<span class="bullet_point_description">Override this function to change the default physical device features.
Each argument is a non-const reference to a vulkan class that allows changing the necessary features by
calling member functions on them. For example, `features10.setDepthClamp(true)` or `features13.setShaderZeroInitializeWorkgroupMemory(true)`.
See the vulkan documentation of {% include link_to_spec_1.3 name="VkPhysicalDeviceFeatures" %},
{% include link_to_spec_1.3 name="VkPhysicalDeviceVulkan11Features" %},
{% include link_to_spec_1.3 name="VkPhysicalDeviceVulkan12Features" %} and
{% include link_to_spec_1.3 name="VkPhysicalDeviceVulkan13Features" %} for a list of things that can be changed;
or consult <span class="command">/usr/include/vulkan/vulkan_structs.hpp</span> for the available member functions.</span>

* `void prepare_logical_device(DeviceCreateInfo& device_create_info) const`
<span class="bullet_point_description">Override this function to add QueueRequest objects.
The default will create a graphics and presentation queue. See below.</span>
{% endcapture %}{% include lv_note.html %}

&nbsp;

Every time `create_root_window` is called a `int` "cookie" must be passed.
This cookie is matched against the cookies used in the virtual
function `prepare_logical_device` of `LogicalDevice` (see above) in order
to determine which presentation queue family to use for that window (and related windows)
and there should be defined in the `LogicalDevice` class.

```cpp
 public:
  static constexpr int root_window_request_cookie = 1;
};
```

When overriding the default `prepare_logical_device` one might also
need a "transfer" cookie for more fine grained control over the use of
a transfer queue family.

The only requirement here is that cookies have different values.

In this example we're not using the cookie because we're not overriding `prepare_logical_device`.
The default simply creates one graphics capable queue (for the command buffer), a presentation queue
(to present the final image) and a transfer queue (to upload textures to the GPU).
The graphics and presentation queue will be combined if possible because that is more efficient
and supported by virtually all hardware anyway.

## {% include link_to_github folder="examples/hello_triangle/" name="TrianglePipelineCharacteristic.h" %}

As discussed [before]({{ page.path | remove:page.name | relative_url }}/01_The_Vulkan_API.html#page_vkcommandbuffer)
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

As described [before]({{ page.path | remove:page.name | relative_url }}/01_The_Vulkan_API.html#dynamic-state) some
state that normally is part of a pipeline can also be configured dynamically; every frame. The downside is
that that takes a few clock cycles extra per frame, but on the other hand it can mean that we need considerably less
different pipelines.

This vector lists the state that we want to be dynamic; that we commit to writing every frame into the
[command buffer]({{ page.path | remove:page.name | relative_url }}/01_The_Vulkan_API.html#dynamic-recording)
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
[VkPipeline]({{ page.path | remove:page.name | relative_url }}/01_The_Vulkan_API.html#page_vkpipeline)
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
[`Window::register_shader_templates`](#page_windowcxx).

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

## {% include link_to_github folder="examples/hello_triangle/" name="Window.h" %}

```cpp
#pragma once

#include <vulkan/SynchronousWindow.h>

class Window : public vulkan::task::SynchronousWindow
{
 public:
  using vulkan::task::SynchronousWindow::SynchronousWindow;
```
Also the `Window` class is using the same constructors as the base class.

<div class="extended-hr"><hr /></div>
```cpp
 private:
  // Define render_pass / attachment objects.
  RenderPass  main_pass{this, "main_pass"};
```
The HelloTriangle application only uses a single
[render pass]({{ page.path | remove:page.name | relative_url }}/01_The_Vulkan_API.html#page_render-pass).

{% capture tip_content %}
A Render Graph is a [directed acyclic graph](http://simonstechblog.blogspot.com/2019/07/render-graph.html)
that can be used to specify the dependency of render passes.
It is a convenient way to manage the rendering especially when using a low level API such as Vulkan.

<img src="{{ '/assets/render_graph.png' | relative_url }}" alt="A render graph" id="img_render_graph" />

How to create a more complicated render graph using linuxviewer will be documented later.
{% endcapture %}{% include lv_note.html %}

<div class="extended-hr"><hr /></div>
```cpp
  // We have two shaders. One for the vertex stage and one for the fragment stage of the pipeline.
  // Initialized by register_shader_templates.
  friend class TrianglePipelineCharacteristic;
  vulkan::shader_builder::ShaderIndex m_shader_vert;
  vulkan::shader_builder::ShaderIndex m_shader_frag;
```
The `Window` stores handles/indices to the two shader templates
(see [<span class="command">Window.cxx</span>](#page_windowcxx) for their definition).

<div class="extended-hr"><hr /></div>
```cpp
  // The pipeline factory that will create our graphics pipeline.
  vulkan::pipeline::FactoryHandle m_pipeline_factory;

  // The graphics pipeline.
  vulkan::Pipeline m_graphics_pipeline;
```
The final two ingredients: the pipeline factory that will create the pipeline that we need,
and the object that represents that pipeline itself. And yes, that means that `m_graphics_pipeline`
is incomplete and not usable until the pipeline factory finished creating it. At the moment this
is polled by testing if `m_graphics_pipeline.handle()` is non-NULL. The part of linuxviewer that
will deal with this in a clean way hasn't been implemented yet.

<div class="extended-hr"><hr /></div>
```cpp
  private:
  //---------------------------------------------------------------------------
  // Override virtual functions of the base class.

  void create_render_graph() override;
  void register_shader_templates() override;
  void create_graphics_pipelines() override;
  void render_frame() override;

  // We're not using textures or vertex buffers.
  void create_textures() override { }
  void create_vertex_buffers() override { }
```

Each `Window` class must override several virtual functions of the base class.
All of the above six functions are pure virtual and therefore *must* be implemented
by the user derived `Window` class: there is no default.

However, because we are not using textures or vertex buffers,
the latter two can remain empty.

<div class="extended-hr"><hr /></div>
```cpp
  //---------------------------------------------------------------------------

  // Called by render_frame.
  void draw_frame();
```
Just a private function, called from `render_frame`. We could have put that code
inside the overridden `render_frame` function too.

## {% include link_to_github folder="examples/hello_triangle/" name="Window.cxx" %}

```cpp
#include "sys.h"
#include "Window.h"
#include "TrianglePipelineCharacteristic.h"
```
This is the file where it all comes together.
As stated above, every compilation unit must begin with including `"sys.h"`.

<div class="extended-hr"><hr /></div>
```cpp
namespace {

// Define the vertex shader, with the coordinates of the
// corners of the triangle, and their colors, hardcoded in.
constexpr std::string_view triangle_vert_glsl = R"glsl(
#version 450

layout(location = 0) out vec3 fragColor;

vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(-0.5, 0.5),
    vec2(0.5, 0.5)
);

vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

void main()
{
  gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
  fragColor = colors[gl_VertexIndex];
}
)glsl";
```
This is the definition of the vertex shader template code
as a C++ [raw-string](https://en.cppreference.com/w/cpp/language/string_literal)
using d-char-sequence `"glsl"` (which is arbitrary).

Because this definition begins with `"#version"`-- linuxviewer will not
make changes to it. It is therefore not really a template; it is the
vertex shader code that will be used as-is by the vertex stage.

{% capture tip_content %}
Using linuxviewer, it is possible to declare objects in the `Window` class,
that represent shader resources (vertex buffers, textures, uniform buffers) that need
to be accessible by certain shaders, and give them some (free to choose) 'string id'.
Then one can refer to them in the shader template code using that string id,
and linuxviewer will take care of generating the required declaration of
GLSL variables, also taking care of enumerations like location, descriptor set and binding
numbers, and replace the string ids with those variable names.
This will be documentated later.
{% endcapture %}{% include lv_note.html %}

Our vertex shader doesn't have many variables: only `gl_VertexIndex` as input
and `gl_Position` and `fragColor` as output.

The vertex shader will be called three times per frame because we'll record
`draw(3, 1, 0, 0)` (see {% include link_to_spec_1.3 name="vkCmdDraw" %}) to the command buffer, every frame.

The value of `gl_VertexIndex` therefore iterate over 0, 1 and 2.
The shader uses that index to read the hardcoded triangle coordinates from
the `positions` array and convert them to a point with [homogeneous coordinates](https://en.wikipedia.org/wiki/Homogeneous_coordinates):
a 4D vector with a z-value of `0.0` and a w-value of `1.0`, which is written
to `gl_Position`. Likewise it reads a corresponding color from the array
`colors` and writes that to `fragColor`.

Now the position and color of each of the three corners of a triangle
has been defined (remember we set the topology to `eTriangleList`).

<div class="extended-hr"><hr /></div>
```cpp
// The fragment shader simply writes the interpolated color to the current pixel.
constexpr std::string_view triangle_frag_glsl = R"glsl(
#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;

void main()
{
  outColor = vec4(fragColor, 1.0);
}
)glsl";

} // namespace
```
Also the fragment shader is specified exactly by starting it with `#version`.
This shader is called once for every fragment that is (partially) overlapped
by our triangle; hence, most likely, a lot more often than three times;
namely equal to the number of pixels in the output image that need to get a color.

{% capture tip_content %}
Note that, although there is a one-on-one relationship between a fragment
and an output pixel, a fragment contains more information then just a color;
it also includes a position, a possible texture coordinate, a depth, and it
might not or only partially influence the color of its output pixel because
it is blended with -- or occluded by -- other fragments.

In the case of the HelloTriangle application, each fragment fully determines
the output color of its associated pixel.
{% endcapture %}{% include info_note.html %}

The input `fragColor` is the interpolated color value between the three corner colors of the triangle,
calculated based on the fragment's position within the triangle.
If the triangle had a texture, the fragment coordinates would first be converted
to texture coordinates by interpolating the three u,v coordinates associated with
the triangle's corners. Then, a 'sampler' is used to determine how multiple texture
pixels (texels) contribute to the color of the fragment.
Note that texture pixels are typically called texels to avoid confusion
with pixels of output attachments.

<div class="extended-hr"><hr /></div>
```cpp
void Window::create_render_graph()
{
  DoutEntering(dc::notice, "Window::create_render_graph() [" << this << "]");
```
The first of four virtual functions that we must implement.

To make debugging easier, and to make the debug output of other functions--
that might be called from this function-- easier to understand, it is a good
idea to begin most functions with a `DoutEntering` line.
Especially functions that are not called every frame, otherwise it might
flood a bit too much. There is no reason not add one here, since
`Window::create_render_graph` is called only once.

Typically you'd write to a debug channel (dc) that allows you
to turn off debug output for certain self-consistent parts of the code
once those work. But the general `dc::notice` is good enough for now.

The idea here is to get an [<span class="command">strace</span>](https://man7.org/linux/man-pages/man1/strace.1.html)-like debug output
that literally prints the call stack of functions being called while the program
is running (don't worry, <span class="command">libcwd</span> is *fast*).

{% capture debug_output %}
STATEFULTASK  : |   Entering AIStatefulTask::signal(0x4 (logical_device_index_available)) [0x561317eb5d10]
STATEFULTASK  : |     | Running state bs_multiplex / SynchronousWindow_acquire_queues [0x561317eb5d10]
...
VULKAN        : |     | Entering SynchronousWindow::prepare_swapchain()
VULKAN        : |     |   Entering Swapchain::prepare(0x561317eb5d10, , { ColorAttachment }, Fifo)
...
VULKAN        : |     |     Created object "m_swapchain.m_acquire_semaphore [0x561317eb5d10]" with handle 0xe7f79a0000000005 and type vk::Semaphore
NOTICE        : |     | Entering Window::create_render_graph() [0x561317eb5d10]
RENDERPASS    : |     |   main_pass->stores(~swapchain)
RENDERPASS    : |     |   Assigning index #0 to attachment "swapchain" of render pass "main_pass".
RENDERPASS    : |     |   Entering AttachmentNode::set_store() [main_pass/swapchain]
RENDERPASS    : |     |   Entering AttachmentNode::set_clear() [main_pass/swapchain]
RENDERPASS    : |     |   Entering RenderGraph::generate()
...
{% endcapture %}{% include debug_output.html %}

<div class="extended-hr"><hr /></div>
```cpp
  // This must be a reference.
  auto& output = swapchain().presentation_attachment();
```
This is how we get a reference to the output attachment that we can present to.
During rendering this will be a different attachment every frame, as we get
[the next unused image]({{ page.path | remove:page.name | relative_url }}/01_The_Vulkan_API.html#page_vkswapchainkhr)
from the swapchain.
The `vulkan::rendergraph::Attachment` that we get here is just a "placeholder" representing the output attachment
in the render graph.

```cpp
  // Define the render graph.
  m_render_graph = main_pass->stores(~output);
```
Here the render graph is created. This is the simplest possible render graph, with only a single attachment (`output`),
and a single `RenderPass` (`main_pass`). The `~` in front of `output` tells the vulkan engine to clear
the output image before rendering into. If we don't do that then only the triangle is drawn and
the background would be rubbish.

Note that this "abuses" C++ syntax by overloading operators; `main_pass` is not a pointer, `stores` isn't really
function here and `output` isn't operated on by the `~`. It is just an attempt to "draw" a directed acyclic graph
using C++ syntax. The graph that belong to the above code is:

<img src="{{ '/assets/hello_triangle_render_graph.png' | relative_url }}" alt="The render graph" id="img_hello_triangle_render_graph" />

```cpp
  // Generate everything.
  m_render_graph.generate(this);
}
```
After having drawn the graph, we still need to process and use it.
If we omit to call this line then we'll probably get a Validation Error in `vkCmdBeginRenderPass`.

<div class="extended-hr"><hr /></div>
```cpp
void Window::register_shader_templates()
{
  DoutEntering(dc::notice, "Window::register_shader_templates() [" << this << "]");
```
This virtual function allows us to register the shader templates that were declared at the beginning of this file.
```cpp
  std::vector<vulkan::shader_builder::ShaderInfo> shader_info = {
    { vk::ShaderStageFlagBits::eVertex,   "triangle.vert.glsl" },
    { vk::ShaderStageFlagBits::eFragment, "triangle.frag.glsl" }
  };
```
First we construct `vulkan::shader_builder::ShaderInfo` object for each shader,
initializing it with the stage it will be used in and a string for diagnostics purposes.
```cpp
  shader_info[0].load(triangle_vert_glsl);
  shader_info[1].load(triangle_frag_glsl);
```
Then the `load` member function is used to load the actual shader (template) code.
You can also pass a `std::filesystem::path` as argument to load the shader code from disk.
```cpp
  auto indices = application().register_shaders(std::move(shader_info));
```
All shaders must be registered with the application object.

{% capture tip_content %}
We use the Application as opposed to the Window, because every shader is unique: a hash is calculated and compilation will
only occur when the hash is different; otherwise the previously compiled shader will be used.
Note that this assumes that if template code is different, then so will be the preprocessed
code (with the shader resource variable declarations etc. already generated), which is obviously
the case. But also that if the template code is the same that then we can reuse a previously
compiled shader, which means that then we assume that also the final shader code is the same.
This is not trivial at all-- theoretically it is possible that different locations, descriptor
set enumerations and/or binding numbers would be used. However, linuxviewer makes sure that
that is not case; this is double efficient because that means we not just have less different
shaders when not needed, it also means that the same shader resources use the same descriptor
set and bindings resulting in a more efficient use of descriptor sets.
{% endcapture %}{% include lv_note.html %}

<div class="extended-hr"><hr /></div>
```cpp
  m_shader_vert = indices[0];
  m_shader_frag = indices[1];
}
```
The function `register_shaders` returns an array with `ShaderIndex`s, here stored as `indices`;
we also could have stored our `m_shader_vert` and `m_shader_frag` in an array and used 0 and 1 to index that array.
Remember that we used these two variables [above](#use_shaders) in the pipeline
characteristic to link them to the pipeline that we'll be creating.

<div class="extended-hr"><hr /></div>
```cpp
void Window::create_graphics_pipelines()
{
  DoutEntering(dc::notice, "Window::create_graphics_pipelines() [" << this << "]");
```
This virtual function allows us to initialize the pipeline factory that must create our pipeline.
```cpp
  m_pipeline_factory = create_pipeline_factory(m_graphics_pipeline, main_pass.vh_render_pass() COMMA_CWDEBUG_ONLY(true));
```
The member `m_pipeline_factory` that we added to our `Window` class is just a handle that can store
information that identifies a factory. We still have to create it, which is what happens here.

Another member, `m_graphics_pipeline`, is passed as the first parameter.
This is an output parameter: eventually the information needed to access the created pipeline will be written to `m_graphics_pipeline`.
The second parameter is the Vulkan Handle to our render pass, which was created in the meantime.
```cpp
  m_pipeline_factory.add_characteristic<TrianglePipelineCharacteristic>(this COMMA_CWDEBUG_ONLY(true));
```
Here the pipeline characteristic is added to the pipeline factory. If there were more than one characteristics
we'd just do more calls like this. Note a different characteristic corresponds to a different *type*, not to an object.
```cpp
   m_pipeline_factory.generate(this);
}
```
As was the case in `Window::create_render_graph`, we have to finish with a call to `generate`,
in this case signaling the factory task that is was fully initialized.

<div class="extended-hr"><hr /></div>
```cpp
void Window::render_frame()
{
  // Start frame.
  start_frame();
```
This virtual function is called every frame, therefore it has no `DoutEntering` line at the beginning as that would
flood the debug output too much. Optionally one could use the special debug channel `dc:vkframe`, normally
[turned off]({{ site.baseurl }}/chapters//02_Getting_started.html#libcwdrc), because that is already used for
debug output printed every frame.

The function `start_frame()` is required. It causes us to advance to the next `FrameResourcesData`, rotating
over the number of available frame resources.

Frame resources, in this context, are Vulkan objects that are "in use" while drawing to or presenting
a frame. In order to avoid stalling it is necessary to use more than one of them, so you can work with
one instance while another is still being used for the previous frame, just like is the case with
swapchain images.

{% capture tip_content %}
Linuxviewer creates multiple "frame resource" instances for the following Vulkan objects:

* Attachments: these render targets and depth-stencil buffers, that are part of the framebuffer object,
  remain "in use" while rendering. The output attachment too and even during presenting, but
  that attachment is part of the swapchain.
* Command buffers and pools: the command buffer stores a series of GPU commands, such as drawing
  or memory operations. Also these stay "in use" even after committing the command buffer to a queue.
* Fences: these are used to ensure proper synchronization between the CPU and GPU,
  preventing the GPU from executing commands on resources that are still in use.
  For example, a Fence is used to signal when a command buffer has completed its execution.
* Certain descriptor sets: descriptor sets that contain a descriptor to a shader resource
  that is changed every frame; for example, certain uniform buffers. Those descriptor sets
  automatically are allocated multiple times and iterated over every new frame in order to
  avoid stalling.

Linuxviewer hides all of this from the user.

The number of frame resources that are created can be changed by overriding the
virtual function `number_of_frame_resources` in your `Window` class.
{% endcapture %}{% include lv_note.html %}

<div class="extended-hr"><hr /></div>
```cpp
  // Acquire swapchain image.
  acquire_image();                    // Can throw vulkan::OutOfDateKHR_Exception.
```
This acquires the next (unused) `vkImage` from the swapchain and does the required internal adjustments.

It is possible that acquiring a swapchain image fails with `vk::Result::eErrorOutOfDateKHR`
because the window was resized and the swapchain images are no longer suited for presentation.
If that happens then this function will throw an exception that is caught elsewhere and
causes the whole swapchain to be regenerated. In other words: linuxviewer also takes care
of window resizing for you.

```cpp
  // Draw scene/prepare scene's command buffers, includes command buffer submission.
  draw_frame();
```
Call our own private function that records and commits the command buffer. See below.

```cpp
  // Draw GUI and present swapchain image.
  finish_frame();
}
```
Does the actual presentation of the rendered image. Also this might fail due
to a window resize in which case an exception is thrown, leading to the
regeneration of the swapchain.

<div class="extended-hr"><hr /></div>
```cpp
void Window::draw_frame()
{
```
This function records the command buffer(s). It assembles all the building blocks
that we have prepared, most notably the correct pipeline(s) and the dynamic state
of those pipelines.

It begins with grabbing a pointer to the current `FrameResourcesData` object:

```cpp
  vulkan::FrameResourcesData* frame_resources = m_current_frame.m_frame_resources;
```

The next line links the appropriate `VkImage`s to our
[imageless framebuffer]({{ page.path | remove:page.name | relative_url }}/01_The_Vulkan_API.html#imageless-framebuffer):

```cpp
  main_pass.update_image_views(swapchain(), frame_resources);
```

<div class="extended-hr"><hr /></div>
```cpp
  auto swapchain_extent = swapchain().extent();

  vk::Viewport viewport{
    .x = 0,
    .y = 0,
    .width = static_cast<float>(swapchain_extent.width),
    .height = static_cast<float>(swapchain_extent.height),
    .minDepth = 0.0f,
    .maxDepth = 1.0f
  };

  vk::Rect2D scissor{
    .offset = vk::Offset2D(),
    .extent = swapchain_extent
  };
```
Here we prepare a `vk::Viewport` and a `vk::Rect2D` for the dynamic state registered
through `m_dynamic_states`, see [above](#dynamic-states).

Note that by making these two dynamic you avoid having to recreate
the pipeline everytime the window is resized (although we still
recreate all attachment- and swapchain images).

<div class="extended-hr"><hr /></div>
```cpp
  wait_command_buffer_completed();
  m_logical_device->reset_fences({ *frame_resources->m_command_buffers_completed });
```
Even though we rotate over multiple command buffers, and other resources that are
used inside them, it can still happen that the command buffer we have to use next
is *still* in use (most notably when you override `number_of_frame_resources`
and have that return `1`). Therefore here we wait until the frame resources that
we're about to (re)use are no longer in use by a previous frame.

Then the fence that is used for this synchronization is reset, so that we
can reuse it to signal us when the command buffer that we're about to record has
completed.

{% capture tip_content %}
If drawing is fast enough we virtually use no CPU in between frames
and the `draw_frame` function could be called very rapidly, creating
(command buffers for) several new frames, until we run out of frame
resources, or out of swapchain images.

In the latter case, blocking occurs during presenting: inside `finish_frame`
that is called at the bottom of `Window::render_frame` as soon as we return
from the current function---because we're using `vk::PresentModeKHR::eFifo`
which waits for VSYNC.

The former would block here, in `wait_command_buffer_completed`.

Neither is very good for latency. In the ideal case you'd want to wait
for user input (i.e. mouse or keyboard) for as long as possible before
starting to render things that depend on them (which is basically everything
when you're -say- rotating the camera) then start to render the frame
and finish *just* in time to present it before the next VSYNC.

In other words, you don't want to call `draw_frame` as fast as possible,
but---depending on how long it takes to render the next frame---wait as
long as possible, so you have time to incorporate the latest user input
into the next frame.

The best guess here is to assume that the next frame will take just as
long as the previous frame; and if that is the case, then the calls to
`draw_frame` will be happen at the same, constant interval as VSYNC;
lets say 60 times per second.

Linuxviewer allows to control the delay at which `draw_frame` is called
by specifying a minimum interval timeval that must have passed between
those calls. If your monitor is 60 FPS and the each frame takes about
as much to render as the previous one, then setting this delay to a little
less than 1/60 of a second makes sense. The default can be controlled
by overriding the virtual function `frame_rate_interval`.

The default interval is currently set to 10 ms, suitable for 60 FPS
monitors. If you have a 120 FPS monitor you might want to override `frame_rate_interval`
and for example `return threadpool::Interval<5, std::chrono::milliseconds>{};`.
{% endcapture %}{% include lv_note.html %}

<div class="extended-hr"><hr /></div>
```cpp
  vulkan::handle::CommandBuffer command_buffer = frame_resources->m_command_buffer;
```
Get the current command buffer to use from the `frame_resources` object.

```cpp
  Dout(dc::vkframe, "Start recording command buffer.");
  command_buffer.begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
  {
```
Begin recording to the command buffer.

The flag specifies that the command buffer will be submitted to a queue for execution
exactly once and should not be reused after that. Using this flag can provide performance
benefits by giving the Vulkan implementation hints on how to optimize memory allocation
and resource management for the command buffer.

```cpp
    command_buffer.beginRenderPass(main_pass.begin_info(), vk::SubpassContents::eInline);
```
Begin the render pass. We only have one render pass in this example.

{% capture tip_content %}
`vk::SubpassContents::eInline` is an enumerator in the Vulkan API that is used to specify the contents of a subpass when you begin recording it within a command buffer. Subpasses are part of Vulkan's render pass structure and allow you to perform various operations on attachments (such as color and depth buffers) within a single render pass.

There are two types of subpass contents:

* `vk::SubpassContents::eInline`: This specifies that the commands used for rendering are to be embedded directly in the primary command buffer, along with any other commands. With this mode, the commands recorded in the primary command buffer are executed sequentially, and no secondary command buffers are used.

* `vk::SubpassContents::eSecondaryCommandBuffers`: This specifies that the rendering commands for this subpass are not directly included in the primary command buffer. Instead, the primary command buffer will reference secondary command buffers that contain the actual rendering commands. This allows you to execute multiple secondary command buffers concurrently, which can help with load balancing and parallelism.

When you begin a subpass using `vk::CommandBuffer::beginRenderPass` in C++ or `beginRenderPass` as above using linuxviewer,
you need to provide a `vk::SubpassContents` value to indicate the contents of the subpass.
Choosing `vk::SubpassContents::eInline` means that the rendering commands will be directly
recorded into the primary command buffer, and you will not use secondary command buffers for this subpass.
This can be useful for simple rendering operations where parallelism and command reusability are not critical factors.
{% endcapture %}{% include info_note.html %}

```cpp
    command_buffer.setViewport(0, { viewport });
    command_buffer.setScissor(0, { scissor });
```
Record the dynamic state regarding the viewport and scissors into the command buffer.
```cpp
    if (m_graphics_pipeline.handle())
    {
      command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, vh_graphics_pipeline(m_graphics_pipeline.handle()));
      command_buffer.draw(3, 1, 0, 0);
    }
    else
      Dout(dc::warning, "Pipeline not available");
```
As stated before, at the moment we need to check if the pipeline was already created by
testing the value of `m_graphics_pipeline.handle()`. This will be fixed later at which
point this documentation will also be updated.

Once the pipeline has become available we first bind the it to the command buffer,
and then record a `draw` call that will cause 3 vertices and 1 instance to be drawn,
starting with `gl_VertexIndex = 0` and `gl_InstanceIndex = 0`.

<div class="extended-hr"><hr /></div>
```cpp
    command_buffer.endRenderPass();
  }
  command_buffer.end();
  Dout(dc::vkframe, "End recording command buffer.");

  submit(command_buffer);
}
```
Here we first mark the end of the render pass, then complete the
recording of the command buffer by calling `command_buffer.end()`.

Finally, the command buffer is submitted to a queue for execution.
