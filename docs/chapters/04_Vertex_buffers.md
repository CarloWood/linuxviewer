---
layout: chapter
title: Vertex buffers
utterance_id: vertex_buffers
---
* TOC
{:toc}

{% assign linuxviewer_api = site.baseurl | append: '/chapters/03_Drawing_a_triangle/02_linuxviewer_API' -%}

## Introduction ##

Change directory into <span class="command">linuxviewer/src/examples</span>
and change git branch to <span class="command">'hello_vertex_buffer'</span>
by typing:
```bash
git switch hello_vertex_buffer
```

The directory <span class="command">hello_triangle/</span> disappeared and
a new directory <span class="command">hello_vertex_buffer/</span> has appeared.

This new directory contains an example, based on <span class="command">hello_triangle</span>,
but with added support for a vertex buffer.

In this chapter we will describe the changes that were made.

{% capture tip_content %}
You can view all those changes as a <span class="command">diff</span> with the command:
```bash
git diff hello_triangle..hello_vertex_buffer
```
while inside the <span class="command">example/</span> directory.
You might want to do that before reading this chapter.
{% endcapture %}{% include lv_note.html %}

{% capture tip_content %}
While inside the example directory, type
```bash
git branch
```
to see an overview of all examples.
{% endcapture %}{% include info_note.html %}

## hello_vertex_buffer ##

### VertexData.h ###

Our vertex buffer contains per-vertex data, and will replace the hardcoded
values in the vertex shader: the position and color of each vertex.

The way linuxviewer works is that you define a structure---that has to be
filled on the CPU and used by the GPU---using provided macros, allowing
linuxviewer to deduce the exact layout and make sure that all alignments
and offsets are correct. The memory layout used by the GPU is a function of
where the data is used (vertex buffers, or uniform buffers...) and is not
the same as the normal C++ layout of structs.

<div class="extended-hr"><hr /></div>
```cpp
#pragma once

#include <vulkan/shader_builder/VertexAttribute.h>

struct VertexData;
```
The struct must be forward declared. The `pragma once` and `include` speak for themselves.
```cpp
LAYOUT_DECLARATION(VertexData, per_vertex_data)
{
  static constexpr auto struct_layout = make_struct_layout(
    LAYOUT(vec2, m_position),
    LAYOUT(vec3, m_color)
  );
};
```
This teaches the vulkan engine the layout of the struct `VertexData`, which is defined below.

The types used, `vec2` and `vec3` are equivalent to the types
used in that C++ struct, but without the `glsl` namespace.

Note that we explicitly specify that this is `per_vertex_data`,
which both, influences the memory layout and specifies the input rate;
the other option for vertex buffers is `per_instance_data`.

```cpp
// Struct describing data type and format of vertex attributes.
struct VertexData
{
  glsl::vec2 m_position;
  glsl::vec3 m_color;
};
```
This is the actual C++ struct with the per-vertex data.
We're going to make a vertex buffer with three of these,
as if an array `VertexData buffer[3];`.

The position here is stored as a `vec2` because, just
like it was hardcoded in the vertex shader, we only
store an x- and y-value.

The color is stored as a `vec3`, a vector of three floats for the RGB values,
also just like as it was hardcoded in the shader.

Note that like this the data will be interleaved, alternating
position and colors. It would totally be possible to make
two vertex buffers, one with the position and one with the
colors; in which case it more closely resembles the layout
used in <span class="command">hello_triangle</span>.

### Triangle.h ###

The `Triangle` class is derived from `vulkan::shader_builder::VertexShaderInputSet`,
which in turn is derived from `vulkan::DataFeeder`.
It is used to fill our array of `VertexData` objects.

```cpp
#pragma once

#include "VertexData.h"
#include "vulkan/shader_builder/VertexShaderInputSet.h"

class Triangle final : public vulkan::shader_builder::VertexShaderInputSet<VertexData>
{
  // A batch exists of one triangle, or three vertices.
  //
  //                 A
  //                / \
  //               /   \
  //              /     \
  //             B-------C
  //
  static constexpr int number_of_vertices = 3;

  // The positions of the triangle corners are stored as 2D vectors; the z and w value are set in the shader.
  static Eigen::Vector2f const s_position[number_of_vertices];
  // Colors are stored as a triplet of three float values for Red, Green and Blue respectively.
  static Eigen::Vector3f const s_color[number_of_vertices];
```
The `DataFeeder` works as follows: first the virtual function `chunk_count()` is called
to obtain the total number of `VertexData` objects in the vertex buffer.
That is, in the case of `VertexShaderInputSet`, `chunk_count()` returns a vertex count. Then `next_batch()` and `get_chunks()`
are called in a loop until all data has been retrieved. Here `next_batch()`
returns the number of chunks (`VertexData` objects in our case) that will
be filled at once by the next call to `get_chunks()`.

```cpp
 private:
  // Returns the total number of VertexData objects in the vertex buffer.
  int chunk_count() const override
  {
    return number_of_vertices;
  }

  // Returns the number of VertexData objects that are initialized by a single call to create_entry.
  int next_batch() override
  {
    return number_of_vertices;
  }
```
In this case we have `number_of_vertices` vertices, returned by `chunk_count()` and
we'll fill all data in one go, hence that `next_batch()` returns the full amount.

```cpp
  // Initialize the next `number_of_vertices` VertexData objects.
  void create_entry(VertexData* input_entry_ptr) override
  {
    for (int vertex = 0; vertex < number_of_vertices; ++vertex)
    {
      input_entry_ptr[vertex].m_position = s_position[vertex];
      input_entry_ptr[vertex].m_color = s_color[vertex];
    }
  }
};
```
This last function copies the data from the static tables into the `VertexData` objects
of the vertex buffer.

### Triangle.cxx ###

```cpp
#include "sys.h"
#include "Triangle.h"

Eigen::Vector2f const Triangle::s_position[number_of_vertices] = {
  { 0.0f, -0.5f },      // A
  { -0.5f, 0.5f },      // B
  { 0.5f, 0.5f },       // C
};

Eigen::Vector3f const Triangle::s_color[number_of_vertices] = {
  { 1.0f, 0.0f, 0.0f },  // A
  { 0.0f, 1.0f, 0.0f },  // B
  { 0.0f, 0.0f, 1.0f },  // C
};
```
These are the [same values]({{ linuxviewer_api }}/07_Window_cxx.html#shaders) as we had hardcoded into the vertex shader at first.

### Window.h ###
Add additional required headers:
```diff
 #pragma once
 
+#include "Triangle.h"
+#include <vulkan/VertexBuffers.h>
 #include <vulkan/SynchronousWindow.h>
```
Add to your `Window` class the member `vulkan::VertexBuffers m_vertex_buffers;`, which
will represent all vertex buffers. And add a member `Triangle m_triangle;`, the class
that we added above, that we will use to fill our vertex buffer:
```diff
   vulkan::shader_builder::ShaderIndex m_shader_vert;
   vulkan::shader_builder::ShaderIndex m_shader_frag;
 
+  // Vertex buffers.
+  vulkan::VertexBuffers m_vertex_buffers;
+
+  // Vertex buffer generators.
+  Triangle m_triangle;
+
```
Now that we have a vertex buffer, we need to implement `create_vertex_buffers()`
instead of leaving it empty:
```diff
   void create_render_graph() override;
+  void create_vertex_buffers() override;
   void register_shader_templates() override;
   void create_graphics_pipelines() override;
   void render_frame() override;
 
-  // We're not using textures or vertex buffers.
+  // We're not using textures.
   void create_textures() override { }
-  void create_vertex_buffers() override { }
```

### Window.cxx ###
```diff
 #include "Window.h"
 #include "TrianglePipelineCharacteristic.h"
+
+// Required header to be included below all other headers (that might include vulkan headers).
+#include <vulkan/lv_inline_definitions.h>
```
Perhaps we should have done this before, because `Window.cxx` is a .cxx file and it uses
linuxviewer objects. We could get away with omitting this header because nothing was
using inline functions, but after adding the vertex buffer this compilation unit stops
compiling without this header.

{% capture tip_content %}
If you forget to include `<vulkan/lv_inline_definitions.h>` then the first time
you compile you might get a *warning*, after that the only thing left might be
a linker error. In this case, when omitting the header, we get:

```bash
/usr/bin/ld: src/examples/hello_vertex_buffer/CMakeFiles/hello_vertex_buffer.dir/Window.cxx.o: in function `Window::create_vertex_buffers()':
src/examples/hello_vertex_buffer/Window.cxx:55: undefined reference to `void vulkan::VertexBuffers::create_vertex_buffer<VertexData>(vulkan::task::SynchronousWindow const*, vulkan::shader_builder::VertexShaderInputSet<VertexData>&)'
/usr/bin/ld: src/examples/hello_vertex_buffer/CMakeFiles/hello_vertex_buffer.dir/Window.cxx.o: in function `Window::draw_frame()':
src/examples/hello_vertex_buffer/Window.cxx:131: undefined reference to `vulkan::handle::CommandBuffer::bindVertexBuffers(vulkan::VertexBuffers const&)'
```

Make sure you recognize the error, so you know what you forgot when you run into it.
Unfortunately, the missing functions might be different, so all you can go on is the `undefined reference` here.

Therefore, pay attention to the warning, which is much clearer about what is missing:
```bash
src/vulkan/CommandBuffer.h:85:38: warning: inline function 'vulkan::handle::CommandBuffer::bindVertexBuffers' is not defined [-Wundefined-inline]
  [[gnu::always_inline]] inline void bindVertexBuffers(VertexBuffers const& vertex_buffers);
                                     ^
/home/carlo/projects/aicxx/linuxviewer/linuxviewer/src/examples/hello_vertex_buffer/Window.cxx:131:20: note: used here
    command_buffer.bindVertexBuffers(m_vertex_buffers);
                   ^
```
{% endcapture %}{% include lv_important.html %}

We need to remove the `#version` line from the shader code, otherwise we can't
make use of the automatic generation of the vertex buffer declaration.
If a `#version` line is present then linuxviewer won't touch the shader code at all.
```diff
 constexpr std::string_view triangle_vert_glsl = R"glsl(
-#version 450
-
```
Remove the hardcoded values from the vertex shader:
```diff
-vec2 positions[3] = vec2[](
-    vec2(0.0, -0.5),
-    vec2(-0.5, 0.5),
-    vec2(0.5, 0.5)
-);
-
-vec3 colors[3] = vec3[](
-    vec3(1.0, 0.0, 0.0),
-    vec3(0.0, 1.0, 0.0),
-    vec3(0.0, 0.0, 1.0)
-);
-
 void main()
```
And then simply refer to the `m_position` and `m_color` members of our `VertexData` structure as follows:
```diff
-  gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
-  fragColor = colors[gl_VertexIndex];
+  gl_Position = vec4(VertexData::m_position, 0.0, 1.0);
+  fragColor = VertexData::m_color;
```
{% capture tip_content %}
Note that the *type* `VertexData` corresponds to an instance of a vertex buffer.
If you have more than one vertex buffer, they all need to use a different type for the data that they contain.
{% endcapture %}{% include lv_important.html %}

Every time `main()` of the vertex shader is entered, a different `VertexData` element of the vertex buffer
will be accessed, iterating over those elements in the vertex buffer as specified by the
[`draw`]({{ linuxviewer_api }}/07_Window_cxx.html#draw) call recorded in the command buffer.

If there is more than one instance, by providing a `per_instance_data` vertex buffer (see above), then the
process starts over, once for each instance (as specified by the
[`draw`]({{ linuxviewer_api }}/07_Window_cxx.html#draw) call recorded in the command buffer).

In other words, it is as if the vertex shader function is inside a loop that runs over all
`per_vertex_data` elements of a vertex buffer, that itself is inside a loop that runs over all
`per_instance_data` elements of a "per instance" vertex buffer. In reality this is not done sequentially
of course, but massively in parallel.

<div class="extended-hr"><hr /></div>
```diff
+void Window::create_vertex_buffers()
+{
+  DoutEntering(dc::vulkan, "Window::create_vertex_buffers() [" << this << "]");
+
+  m_vertex_buffers.create_vertex_buffer(this, m_triangle);
+}
+
 void Window::register_shader_templates()
```
This is the implementation of the---now---overridden virtual function `create_vertex_buffers()`.
All it does is call `create_vertex_buffer` on the `vulkan::VertexBuffers` member that we
added to our `Window` class, passing the `this` pointer to our window, and the vertex buffer
generator `m_triangle`.

Finally, change `Window::draw_frame` to bind the vertex buffer(s) to the pipeline:
```diff
     command_buffer.beginRenderPass(main_pass.begin_info(), vk::SubpassContents::eInline);
     command_buffer.setViewport(0, { viewport });
     command_buffer.setScissor(0, { scissor });
+    command_buffer.bindVertexBuffers(m_vertex_buffers);
 
```

### TrianglePipelineCharacteristic.h ###
```diff
   TrianglePipelineCharacteristic(vulkan::task::SynchronousWindow const* owning_window COMMA_CWDEBUG_ONLY(bool debug)) :
     vulkan::pipeline::Characteristic(owning_window COMMA_CWDEBUG_ONLY(debug))
   {
-    // Required when we don't have vertex buffers.
-    m_use_vertex_buffers = false;
   }
```
No longer set `m_use_vertex_buffers` to false!

{% capture tip_content %}
If you leave this in (but make all other changes) you'll get the following `ASSERT`:

{% capture debug_output %}
ThreadPool01    COREDUMP      : |     linuxviewer/src/vulkan/pipeline/AddShaderStage.cxx:175: void vulkan::pipeline::AddShaderStage::preprocess1(const shader_builder::ShaderInfo &): Assertion `!m_shader_variables.empty()' failed.
{% endcapture %}{% include debug_output.html %}
Whenever you run into an assert, carefully read the comment above the assert.
With a bit of luck it will explain what you did wrong.

In this case that comment contains,

```cpp
//   Perhaps you (still) have `m_use_vertex_buffers = false;` in the constructor of
//   (one of) your pipeline Characteristic? If so, start with removing that line.
```
{% endcapture %}{% include info_note.html %}

At the bottom of the `initialize()` member function add:
```diff
     add_shader(window->m_shader_frag);
+
+    // Register the vertex buffer.
+    add_vertex_input_bindings(window->m_vertex_buffers);
   }
```

### HelloTriangle.h ###
```diff
   std::u8string application_name() const override
   {
-    return u8"Hello Triangle";
+    return u8"Hello Vertex Buffer";
   }
```
That was the last change that we made. Added for completeness, as it is clearly not related to adding a vertex buffer.

## hello_two_vertex_buffers ##

Instead of using one vertex buffer, `VertexData` (remember: one type corresponds to one vertex buffer),
we could have used --say-- `VertexPosition` and `VertexColor`, separating positions and colors into two vertex buffers:

### VertexPosition.h ###

```cpp
#pragma once

#include <vulkan/shader_builder/VertexAttribute.h>

struct VertexPosition;

LAYOUT_DECLARATION(VertexPosition, per_vertex_data)
{
  static constexpr auto struct_layout = make_struct_layout(
    LAYOUT(vec2, m_position)
  );
};

// Struct describing data type and format of vertex attributes.
struct VertexPosition
{
  glsl::vec2 m_position;
};
```

### VertexColor.h ###

```cpp
#pragma once

#include <vulkan/shader_builder/VertexAttribute.h>

struct VertexColor;

LAYOUT_DECLARATION(VertexColor, per_vertex_data)
{
  static constexpr auto struct_layout = make_struct_layout(
    LAYOUT(vec3, m_color)
  );
};

// Struct describing data type and format of vertex attributes.
struct VertexColor
{
  glsl::vec3 m_color;
};
```

And then have two vertex buffer generators, for example `Position` and `Color`, instead of `Triangle`, that
only fill respectively `m_position` and `m_color` of the above structs.

Define both in your `Window` class:
```diff
   // Vertex buffer generators.
-  Triangle m_triangle;                  // Vertex buffer.
+  Position m_position;
+  Color m_color;
```
and add both to `m_vertex_buffers`:
```diff
-  m_vertex_buffers.create_vertex_buffer(this, m_triangle);
+  m_vertex_buffers.create_vertex_buffer(this, m_position);
+  m_vertex_buffers.create_vertex_buffer(this, m_color);
```

And change the shader code to use `VertexPosition::m_position` and `VertexColor::m_color`
instead of `VertexData::` for both:
```diff
 void main()
 {
-  gl_Position = vec4(VertexData::m_position, 0.0, 1.0);
-  fragColor = VertexData::m_color;
+  gl_Position = vec4(VertexPosition::m_position, 0.0, 1.0);
+  fragColor = VertexColor::m_color;
 }
```
