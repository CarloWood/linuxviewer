#pragma once

#include <vulkan/SynchronousWindow.h>

class Window : public vulkan::task::SynchronousWindow
{
 public:
  using vulkan::task::SynchronousWindow::SynchronousWindow;

 private:
  // Define renderpass / attachment objects.
  RenderPass  main_pass{this, "main_pass"};

  // We have two shaders. One for the vertex stage and one for the fragment stage of the pipeline.
  // Initialized by register_shader_templates.
  friend class TrianglePipelineCharacteristic;
  vulkan::shader_builder::ShaderIndex m_shader_vert;
  vulkan::shader_builder::ShaderIndex m_shader_frag;

  // The pipeline factory that will create our graphics pipeline.
  vulkan::pipeline::FactoryHandle m_pipeline_factory;

  // The graphics pipeline.
  vulkan::Pipeline m_graphics_pipeline;

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

  //---------------------------------------------------------------------------

  // Called by render_frame.
  void draw_frame();
};
