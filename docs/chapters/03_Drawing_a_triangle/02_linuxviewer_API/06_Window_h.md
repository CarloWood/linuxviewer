---
layout: chapter
title: Linuxviewer API
toc_title: Window.h
utterance_id: lv_API_Window_h_draft
---
{% assign drawing_a_triangle = site.baseurl | append: '/chapters/03_Drawing_a_triangle' -%}

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
[render pass]({{ drawing_a_triangle }}/01_The_Vulkan_API.html#page_render-pass).

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
(see [<span class="command">Window.cxx</span>]({{ page.path | remove:page.name | relative_url }}/07_Window_cxx.html#shaders)
for their definition).

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
