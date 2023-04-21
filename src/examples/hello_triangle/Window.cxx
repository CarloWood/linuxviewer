#include "sys.h"
#include "Window.h"
#include "TrianglePipelineCharacteristic.h"

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

void Window::create_render_graph()
{
  DoutEntering(dc::notice, "Window::create_render_graph() [" << this << "]");

  // This must be a reference.
  auto& output = swapchain().presentation_attachment();

  // Define the render graph.
  m_render_graph = main_pass->stores(~output);

  // Generate everything.
  m_render_graph.generate(this);
}

void Window::register_shader_templates()
{
  DoutEntering(dc::notice, "Window::register_shader_templates() [" << this << "]");
  std::vector<vulkan::shader_builder::ShaderInfo> shader_info = {
    { vk::ShaderStageFlagBits::eVertex,   "triangle.vert.glsl" },
    { vk::ShaderStageFlagBits::eFragment, "triangle.frag.glsl" }
  };
  shader_info[0].load(triangle_vert_glsl);
  shader_info[1].load(triangle_frag_glsl);
  auto indices = application().register_shaders(std::move(shader_info));
  m_shader_vert = indices[0];
  m_shader_frag = indices[1];
}

void Window::create_graphics_pipelines()
{
  DoutEntering(dc::notice, "Window::create_graphics_pipelines() [" << this << "]");
  m_pipeline_factory = create_pipeline_factory(m_graphics_pipeline, main_pass.vh_render_pass() COMMA_CWDEBUG_ONLY(true));
  m_pipeline_factory.add_characteristic<TrianglePipelineCharacteristic>(this COMMA_CWDEBUG_ONLY(true));
  m_pipeline_factory.generate(this);
}

//===========================================================================
//
// Frame code (called every frame)
//
//===========================================================================

void Window::render_frame()
{
  // Start frame.
  start_frame();

  // Acquire swapchain image.
  acquire_image();                    // Can throw vulkan::OutOfDateKHR_Exception.

  // Draw scene/prepare scene's command buffers, includes command buffer submission.
  draw_frame();

  // Draw GUI and present swapchain image.
  finish_frame();
}

void Window::draw_frame()
{
  vulkan::FrameResourcesData* frame_resources = m_current_frame.m_frame_resources;
  main_pass.update_image_views(swapchain(), frame_resources);
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

  wait_command_buffer_completed();
  m_logical_device->reset_fences({ *frame_resources->m_command_buffers_completed });

  vulkan::handle::CommandBuffer command_buffer = frame_resources->m_command_buffer;
  Dout(dc::vkframe, "Start recording command buffer.");
  command_buffer.begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
  {
    command_buffer.beginRenderPass(main_pass.begin_info(), vk::SubpassContents::eInline);
    command_buffer.setViewport(0, { viewport });
    command_buffer.setScissor(0, { scissor });

    // FIXME: this is a hack - what we really need is a vector with RenderProxy objects.
    // For now we must make sure that the pipeline was created before drawing to it.
    if (m_graphics_pipeline.handle())
    {
      command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, vh_graphics_pipeline(m_graphics_pipeline.handle()));
      command_buffer.draw(3, 1, 0, 0);
    }
    else
      Dout(dc::warning, "Pipeline not available");

    command_buffer.endRenderPass();
  }
  command_buffer.end();
  Dout(dc::vkframe, "End recording command buffer.");

  submit(command_buffer);
}
