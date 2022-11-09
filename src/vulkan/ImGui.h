#pragma once

#include "CommandBuffer.h"      // handle::CommandBuffer
#include "FrameResourcesData.h" // vulkan::FrameResourcesData::command_pool_type::data_type::create_flags
#include "lvimconfig.h"         // lvImGuiTLS
#include "CurrentFrameData.h"
#include "memory/Buffer.h"
#include "shader_builder/ShaderIndex.h"
#include "shader_builder/VertexAttribute.h"
#include "shader_builder/VertexShaderInputSet.h"
#include "shader_builder/shader_resource/Texture.h"
#include "vk_utils/TimerData.h"
#ifdef CWDEBUG
#include "debug/DebugSetName.h"
#endif
#include "debug.h"

// Theoretically we should include imgui.h to get the definition of ImDrawVert.
// However - I think that any other class will do here, as long as it has the
// same memory layout. We can even put this dummy struct in a namespace.

namespace imgui {
struct ImDrawVert;
} // namespace imgui

// Describe the ImDrawVert's vertex input attribute layouts in terms of this dummy struct,
// since it only uses things like sizeof and offsetof.
LAYOUT_DECLARATION(imgui::ImDrawVert, per_vertex_data)
{
  static constexpr auto struct_layout = make_struct_layout(
    LAYOUT(vec2, pos),
    LAYOUT(vec2, uv),
    LAYOUT(u8vec4, col)
  );
};

namespace imgui {

struct ImDrawVert
{
  glsl::vec2 pos;
  glsl::vec2 uv;
  uint32_t col;
};

using ImDrawIdx = unsigned short;

} // namespace imgui

namespace imgui {

class UI final : public vulkan::shader_builder::VertexShaderInputSet<ImDrawVert>
{
  int chunk_count() const override
  {
    // This should never be called because the backend writes directly to the vertex buffer. See ImGui::render_frame.
    // Currently this means we simply don't call ShaderInputData::generate.
    ASSERT(false);
    return 0;
  }

  void create_entry(ImDrawVert* input_entry_ptr) override
  {
  }
};

} // namespace imgui

namespace task {
class SynchronousWindow;
} // namespace task

namespace vulkan {

// Frame resources.
struct ImGui_FrameResourcesData
{
  memory::Buffer m_vertex_buffer;
  memory::Buffer m_index_buffer;
  imgui::ImDrawVert* m_mapped_vertex_buffer;
  imgui::ImDrawIdx* m_mapped_index_buffer;
};

class ImGui
{
 private:
  static constexpr auto pool_type = static_cast<vk::CommandPoolCreateFlags::MaskType>(vulkan::FrameResourcesData::command_pool_type::create_flags);

  task::SynchronousWindow const* m_owning_window;
  shader_builder::shader_resource::Texture m_font_texture{"ImGui::m_font_texture"};
  utils::Vector<ImGui_FrameResourcesData, FrameResourceIndex> m_frame_resources_list;
  vk::UniqueDescriptorSetLayout m_descriptor_set_layout;
  vk::DescriptorSet m_vh_descriptor_set;                // Lifetime is determined by the pool (LogicalDevice::m_descriptor_pool).
  vk::UniquePipelineLayout m_pipeline_layout;
  vk::UniquePipeline m_graphics_pipeline;
  std::filesystem::path m_ini_filename;                 // Cache that io.IniFilename points to.
  ImGuiContext* m_context;                              // The ImGui context of this ImGui instance.
  shader_builder::ShaderIndex m_shader_vert;
  shader_builder::ShaderIndex m_shader_frag;
#ifdef CWDEBUG
  int m_last_x = {};
  int m_last_y = {};
#endif

  // Define pipeline objects.
  imgui::UI m_ui;                                       // UI vertex attributes.

 private:
  inline LogicalDevice const* logical_device() const;

  void setup_render_state(handle::CommandBuffer command_buffer, void* draw_data_void_ptr, ImGui_FrameResourcesData& frame_resources, vk::Viewport const& viewport);
  void register_shader_templates();
  void create_descriptor_set(
      CWDEBUG_ONLY(Ambifix const& ambifix));
  void create_graphics_pipeline(vk::SampleCountFlagBits MSAASamples
      COMMA_CWDEBUG_ONLY(Ambifix const& ambifix));

 public:
  void create_frame_resources(FrameResourceIndex number_of_frame_resources
    COMMA_CWDEBUG_ONLY(Ambifix const& ambifix));

  // Signals owning_window with imgui_font_texture_ready when uploading the font texture was finished.
  void init(task::SynchronousWindow* owning_window, vk::SampleCountFlagBits MSAASamples, AIStatefulTask::condition_type imgui_font_texture_ready, GraphicsSettingsPOD const& graphics_settings
      COMMA_CWDEBUG_ONLY(Ambifix const& ambifix));

  // Called at the start of the render loop.
  void set_current_context() { lvImGuiTLS = m_context; }

  void on_window_size_changed(vk::Extent2D extent);
  void on_focus_changed(bool in_focus) const;
  void on_mouse_move(int x, int y);
  void on_mouse_wheel_event(float delta_x, float delta_y) const;
  void on_mouse_click(uint8_t button, bool pressed) const;
  void on_mouse_enter(int x, int y, bool entered) const;
  void on_key_event(uint32_t keysym, bool pressed) const;
  void update_modifiers(int modifiers) const;
  bool want_capture_keyboard() const;
  bool want_capture_mouse() const;

  void start_frame(float delta_s);
  void render_frame(handle::CommandBuffer command_buffer, FrameResourceIndex index
      COMMA_CWDEBUG_ONLY(Ambifix const& ambifix));

  ~ImGui();
};

} // namespace vulkan

struct ImGuiIO;

namespace imgui {

class StatsWindow
{
  bool m_show_fps = true;       // To show FPS or ms.

 public:
  void draw(ImGuiIO& io, vk_utils::TimerData const& timer);
};

} // namespace imgui
