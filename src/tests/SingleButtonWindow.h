#pragma once

#include <vulkan/SynchronousWindow.h>
#include <imgui.h>
#include <functional>
#include "debug.h"

#define ADD_STATS_TO_SINGLE_BUTTON_WINDOW 0

class SingleButtonWindow : public vulkan::task::SynchronousWindow
{
  std::function<void(SingleButtonWindow&)> m_callback;

#if ADD_STATS_TO_SINGLE_BUTTON_WINDOW
  imgui::StatsWindow m_imgui_stats_window;
#endif

 public:
  SingleButtonWindow(std::function<void(SingleButtonWindow&)> callback, vulkan::Application* application COMMA_CWDEBUG_ONLY(bool debug)) :
    vulkan::task::SynchronousWindow(application COMMA_CWDEBUG_ONLY(debug)), m_callback(callback) { }

 private:
  void create_render_graph() override
  {
    DoutEntering(dc::vulkan, "SingleButtonWindow::create_render_graph() [" << this << "]");

    // This must be a reference.
    auto& output = swapchain().presentation_attachment();

    // This window draws nothing but an ImGui window.
    m_render_graph = imgui_pass->stores(~output);

    // Generate everything.
    m_render_graph.generate(this);
  }

  void create_textures() override { }
  void create_vertex_buffers() override { }
  void create_graphics_pipelines() override { }

  //===========================================================================
  //
  // Called from initialize_impl.
  //
  threadpool::Timer::Interval frame_rate_interval() const override
  {
    // Limit the frame rate of this window to 11.111 frames per second.
    return threadpool::Interval<90, std::chrono::milliseconds>{};
  }

  //===========================================================================
  //
  // Frame code (called every frame)
  //
  //===========================================================================

  void render_frame() override
  {
    DoutEntering(dc::vkframe, "SingleButtonWindow::render_frame() [" << this << "]");

    start_frame();
    acquire_image();                    // Can throw vulkan::OutOfDateKHR_Exception.

    vulkan::FrameResourcesData* frame_resources = m_current_frame.m_frame_resources;
    imgui_pass.update_image_views(swapchain(), frame_resources);

    m_logical_device->reset_fences({ *frame_resources->m_command_buffers_completed });
    auto command_buffer = frame_resources->m_command_buffer;

    Dout(dc::vkframe, "Start recording command buffer.");
    command_buffer.begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
    command_buffer.beginRenderPass(imgui_pass.begin_info(), vk::SubpassContents::eInline);
    m_imgui.render_frame(command_buffer, m_current_frame.m_resource_index COMMA_CWDEBUG_ONLY(debug_name_prefix("m_imgui")));
    command_buffer.endRenderPass();
    command_buffer.end();
    Dout(dc::vkframe, "End recording command buffer.");

    submit(command_buffer);

    // Draw GUI and present swapchain image.
    finish_frame();
  }

  //===========================================================================
  //
  // ImGui
  //
  //===========================================================================

  void draw_imgui() override final
  {
    ZoneScopedN("SingleButtonWindow::draw_imgui");
    ImGuiIO& io = ImGui::GetIO();

    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::Begin("SingleButton", nullptr,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings);

    if (ImGui::Button("Trigger Event", ImVec2(150.f - 16.f, 50.f - 16.f)))
    {
      Dout(dc::notice, "SingleButtonWindow: calling m_callback() [" << this << "]");
      m_callback(*this);
    }

    ImGui::End();

#if ADD_STATS_TO_SINGLE_BUTTON_WINDOW
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 120.0f, 20.0f));
    m_imgui_stats_window.draw(io, m_timer);
#endif
  }
};
